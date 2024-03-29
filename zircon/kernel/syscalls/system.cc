// Copyright 2017 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <debug.h>
#include <lib/cmdline.h>
#include <lib/debuglog.h>
#include <mexec.h>
#include <platform.h>
#include <string.h>
#include <trace.h>
#include <zircon/boot/image.h>
#include <zircon/compiler.h>
#include <zircon/syscalls/resource.h>
#include <zircon/syscalls/system.h>
#include <zircon/types.h>

#include <arch/arch_ops.h>
#include <arch/mp.h>
#include <dev/interrupt.h>
#include <kernel/mp.h>
#include <kernel/range_check.h>
#include <kernel/thread.h>
#include <libzbi/zbi-cpp.h>
#include <object/event_dispatcher.h>
#include <object/job_dispatcher.h>
#include <object/process_dispatcher.h>
#include <object/resource.h>
#include <object/user_handles.h>
#include <object/vm_object_dispatcher.h>
#include <platform/halt_helper.h>
#include <vm/physmap.h>
#include <vm/pmm.h>
#include <vm/vm.h>
#include <vm/vm_aspace.h>

#include "priv.h"
#include "system_priv.h"

#define LOCAL_TRACE 0

// Allocate this many extra bytes at the end of the bootdata for the platform
// to fill in with platform specific boot structures.
const size_t kBootdataPlatformExtraBytes = PAGE_SIZE * 4;

__BEGIN_CDECLS
extern void mexec_asm(void);
extern void mexec_asm_end(void);
__END_CDECLS

/* Allocates a page of memory that has the same physical and virtual addresses.
 */
static zx_status_t identity_page_allocate(fbl::RefPtr<VmAspace>* new_aspace, void** result_addr) {
  zx_status_t result;

  // Start by obtaining an unused physical page. This address will eventually
  // be the physical/virtual address of our identity mapped page.
  paddr_t pa;
  result = pmm_alloc_page(0, &pa);
  if (result != ZX_OK) {
    return ZX_ERR_NO_MEMORY;
  }

  // The kernel address space may be in high memory which cannot be identity
  // mapped since all Kernel Virtual Addresses might be out of range of the
  // physical address space. For this reason, we need to make a new address
  // space.
  fbl::RefPtr<VmAspace> identity_aspace =
      VmAspace::Create(VmAspace::TYPE_LOW_KERNEL, "mexec identity");
  if (!identity_aspace)
    return ZX_ERR_INTERNAL;

  // Create a new allocation in the new address space that identity maps the
  // target page.
  const uint perm_flags_rwx =
      ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE | ARCH_MMU_FLAG_PERM_EXECUTE;
  void* identity_address = (void*)pa;
  result = identity_aspace->AllocPhysical("identity mapping", PAGE_SIZE, &identity_address, 0, pa,
                                          VmAspace::VMM_FLAG_VALLOC_SPECIFIC, perm_flags_rwx);
  if (result != ZX_OK)
    return result;

  *new_aspace = ktl::move(identity_aspace);
  *result_addr = identity_address;

  return ZX_OK;
}

/* Takes all the pages in a VMO and creates a copy of them where all the pages
 * occupy a physically contiguous region of physical memory.
 * TODO(gkalsi): Don't coalesce pages into a physically contiguous region and
 *               just pass a vectored I/O list to the mexec assembly.
 */
static zx_status_t vmo_coalesce_pages(zx_handle_t vmo_hdl, const size_t extra_bytes, paddr_t* addr,
                                      uint8_t** vaddr, size_t* size) {
  DEBUG_ASSERT(addr);
  if (!addr) {
    return ZX_ERR_INVALID_ARGS;
  }

  DEBUG_ASSERT(size);
  if (!size) {
    return ZX_ERR_INVALID_ARGS;
  }

  auto up = ProcessDispatcher::GetCurrent();
  fbl::RefPtr<VmObjectDispatcher> vmo_dispatcher;
  zx_status_t st = up->GetDispatcherWithRights(vmo_hdl, ZX_RIGHT_READ, &vmo_dispatcher);
  if (st != ZX_OK)
    return st;

  fbl::RefPtr<VmObject> vmo = vmo_dispatcher->vmo();

  const size_t vmo_size = vmo->size();

  const size_t num_pages = ROUNDUP(vmo_size + extra_bytes, PAGE_SIZE) / PAGE_SIZE;

  paddr_t base_addr;
  list_node list = LIST_INITIAL_VALUE(list);
  st = pmm_alloc_contiguous(num_pages, PMM_ALLOC_FLAG_ANY, 0, &base_addr, &list);
  if (st != ZX_OK) {
    // TODO(gkalsi): Free pages allocated by pmm_alloc_contiguous pages
    //               and return an error.
    panic("Failed to allocate contiguous memory");
  }

  uint8_t* dst_addr = (uint8_t*)paddr_to_physmap(base_addr);

  st = vmo->Read(dst_addr, 0, vmo_size);
  if (st != ZX_OK) {
    // TODO(gkalsi): Free pages allocated by pmm_alloc_contiguous pages
    //               and return an error.
    panic("Failed to read to contiguous vmo");
  }

  arch_clean_invalidate_cache_range((addr_t)dst_addr, vmo_size);

  *size = num_pages * PAGE_SIZE;
  *addr = base_addr;
  if (vaddr)
    *vaddr = dst_addr;

  return ZX_OK;
}

static fbl::RefPtr<VmObject> stashed_crashlog;
void mexec_stash_crashlog(fbl::RefPtr<VmObject> vmo) { stashed_crashlog = ktl::move(vmo); }

// zx_status_t zx_system_mexec_payload_get
zx_status_t sys_system_mexec_payload_get(zx_handle_t resource, user_out_ptr<void> user_buffer,
                                         size_t buffer_size) {
  // Highly privilidged, only root resource should have access.
  zx_status_t result = validate_resource(resource, ZX_RSRC_KIND_ROOT);
  if (result != ZX_OK) {
    return result;
  }

  // Limit the size of the result that we can return to userspace.
  if (buffer_size > kBootdataPlatformExtraBytes) {
    return ZX_ERR_INVALID_ARGS;
  }

  fbl::AllocChecker ac;
  ktl::unique_ptr<uint8_t[]> buffer;
  buffer.reset(new (&ac) uint8_t[buffer_size]);
  if (!ac.check()) {
    return ZX_ERR_NO_MEMORY;
  }
  memset(buffer.get(), 0, buffer_size);

  // Create a zero length ZBI in the buffer.
  zbi::Zbi image(buffer.get(), buffer_size);
  zbi_result_t zbi_result = image.Reset();
  if (zbi_result != ZBI_RESULT_OK) {
    return ZX_ERR_INTERNAL;
  }

  result = platform_mexec_patch_zbi(buffer.get(), buffer_size);
  if (result != ZX_OK) {
    return result;
  }

  if (stashed_crashlog && stashed_crashlog->size() <= UINT32_MAX) {
    size_t crashlog_len = stashed_crashlog->size();
    uint8_t* bootdata_section;

    zbi_result_t res = image.CreateSection(static_cast<uint32_t>(crashlog_len), ZBI_TYPE_CRASHLOG,
                                           0, 0, reinterpret_cast<void**>(&bootdata_section));

    if (res != ZBI_RESULT_OK) {
      printf("mexec: could not append crashlog\n");
      return ZX_ERR_INTERNAL;
    }

    result = stashed_crashlog->Read(bootdata_section, 0, crashlog_len);
    if (result != ZX_OK) {
      return result;
    }
  }

  return user_buffer.copy_array_to_user(buffer.get(), buffer_size);
}

// zx_status_t zx_system_mexec
zx_status_t sys_system_mexec(zx_handle_t resource, zx_handle_t kernel_vmo,
                             zx_handle_t bootimage_vmo) {
  // TODO(ZX-971): finer grained validation
  zx_status_t result = validate_resource(resource, ZX_RSRC_KIND_ROOT);
  if (result != ZX_OK)
    return result;

  paddr_t new_kernel_addr;
  size_t new_kernel_len;
  result = vmo_coalesce_pages(kernel_vmo, 0, &new_kernel_addr, NULL, &new_kernel_len);
  if (result != ZX_OK) {
    return result;
  }

  // for kernels that are bootdata based (eg, x86-64), the location
  // to find the entrypoint depends on the bootdata format
  paddr_t entry64_addr =
      (get_kernel_base_phys() + sizeof(zbi_header_t) +  // ZBI_TYPE_CONTAINER header
       sizeof(zbi_header_t) +                           // ZBI_TYPE_KERNEL header
       offsetof(zbi_kernel_t, entry));

  paddr_t new_bootimage_addr;
  uint8_t* bootimage_buffer;
  size_t new_bootimage_len;
  result = vmo_coalesce_pages(bootimage_vmo, kBootdataPlatformExtraBytes, &new_bootimage_addr,
                              &bootimage_buffer, &new_bootimage_len);
  if (result != ZX_OK) {
    return result;
  }

  void* id_page_addr = 0x0;
  fbl::RefPtr<VmAspace> aspace;
  result = identity_page_allocate(&aspace, &id_page_addr);
  if (result != ZX_OK) {
    return result;
  }

  LTRACEF("zx_system_mexec allocated identity mapped page at %p\n", id_page_addr);

  thread_migrate_to_cpu(BOOT_CPU_ID);

  // We assume that when the system starts, only one CPU is running. We denote
  // this as the boot CPU.
  // We want to make sure that this is the CPU that eventually branches into
  // the new kernel so we attempt to migrate this thread to that cpu.
  platform_halt_secondary_cpus();

  platform_mexec_prep(new_bootimage_addr, new_bootimage_len);

  dlog_shutdown();

  arch_disable_ints();

  // WARNING
  // It is unsafe to return from this function beyond this point.
  // This is because we have swapped out the user address space and halted the
  // secondary cores and there is no trivial way to bring both of these back.
  vmm_set_active_aspace(reinterpret_cast<vmm_aspace_t*>(aspace.get()));

  // We're going to copy this into our identity page, make sure it's not
  // longer than a single page.
  size_t mexec_asm_length = (uintptr_t)mexec_asm_end - (uintptr_t)mexec_asm;
  DEBUG_ASSERT(mexec_asm_length <= PAGE_SIZE);

  memcpy(id_page_addr, (const void*)mexec_asm, mexec_asm_length);
  arch_sync_cache_range((addr_t)id_page_addr, mexec_asm_length);

  // We must pass in an arg that represents a list of memory regions to
  // shuffle around. We put this args list immediately after the mexec
  // assembly.
  uintptr_t ops_ptr = ((((uintptr_t)id_page_addr) + mexec_asm_length + 8) | 0x7) + 1;
  memmov_ops_t* ops = (memmov_ops_t*)(ops_ptr);

  const size_t num_ops = 2;
  // Make sure that we can also pack the arguments in the same page as the
  // final mexec assembly shutdown code.
  DEBUG_ASSERT(((sizeof(*ops) * num_ops + ops_ptr) - (uintptr_t)id_page_addr) < PAGE_SIZE);

  // Op to move the new kernel into place.
  ops[0].src = (void*)new_kernel_addr;
  ops[0].dst = (void*)get_kernel_base_phys();
  ops[0].len = new_kernel_len;

  // Null terminated list.
  ops[1] = {0, 0, 0};

  // Make sure that the kernel, when copied, will not overwrite the bootdata.
  DEBUG_ASSERT(!Intersects(reinterpret_cast<uintptr_t>(ops[0].dst), ops[0].len,
                           reinterpret_cast<uintptr_t>(new_bootimage_addr), new_bootimage_len));

  // Sync because there is code in here that we intend to run.
  arch_sync_cache_range((addr_t)id_page_addr, PAGE_SIZE);

  // Clean because we're going to turn the MMU/caches off and we want to make
  // sure that things are still available afterwards.
  arch_clean_cache_range((addr_t)id_page_addr, PAGE_SIZE);

  shutdown_interrupts();

  // Ask the platform to mexec into the next kernel.
  mexec_asm_func mexec_assembly = (mexec_asm_func)id_page_addr;
  platform_mexec(mexec_assembly, ops, new_bootimage_addr, new_bootimage_len, entry64_addr);

  panic("Execution should never reach here\n");
  return ZX_OK;
}

// zx_status_t zx_system_powerctl
zx_status_t sys_system_powerctl(zx_handle_t root_rsrc, uint32_t cmd,
                                user_in_ptr<const zx_system_powerctl_arg_t> raw_arg) {
  zx_status_t status;
  if ((status = validate_resource(root_rsrc, ZX_RSRC_KIND_ROOT)) < 0) {
    return status;
  }

  switch (cmd) {
    case ZX_SYSTEM_POWERCTL_ENABLE_ALL_CPUS: {
      cpu_mask_t all_cpus = ((cpu_mask_t)1u << arch_max_num_cpus()) - 1;
      return mp_hotplug_cpu_mask(~mp_get_online_mask() & all_cpus);
    }
    case ZX_SYSTEM_POWERCTL_DISABLE_ALL_CPUS_BUT_PRIMARY: {
      cpu_mask_t primary = cpu_num_to_mask(0);
      return mp_unplug_cpu_mask(mp_get_online_mask() & ~primary);
    }
#if defined __x86_64__
    case ZX_SYSTEM_POWERCTL_ACPI_TRANSITION_S_STATE:
    case ZX_SYSTEM_POWERCTL_X86_SET_PKG_PL1: {
      zx_system_powerctl_arg_t arg;
      MsrAccess msr;
      status = raw_arg.copy_from_user(&arg);
      if (status != ZX_OK) {
        return status;
      }

      return arch_system_powerctl(cmd, &arg, &msr);
    }
#endif  //__x86_64
    case ZX_SYSTEM_POWERCTL_REBOOT:
      platform_graceful_halt_helper(HALT_ACTION_REBOOT);
      break;
    case ZX_SYSTEM_POWERCTL_REBOOT_BOOTLOADER:
      platform_graceful_halt_helper(HALT_ACTION_REBOOT_BOOTLOADER);
      break;
    case ZX_SYSTEM_POWERCTL_REBOOT_RECOVERY:
      platform_graceful_halt_helper(HALT_ACTION_REBOOT_RECOVERY);
      break;
    case ZX_SYSTEM_POWERCTL_SHUTDOWN:
      platform_graceful_halt_helper(HALT_ACTION_SHUTDOWN);
      break;
    default:
      return ZX_ERR_INVALID_ARGS;
  }
  return ZX_OK;
}

// zx_status_t zx_system_get_event
zx_status_t sys_system_get_event(zx_handle_t root_job, uint32_t kind, user_out_handle* out) {
  auto up = ProcessDispatcher::GetCurrent();

  if (kind != ZX_SYSTEM_EVENT_LOW_MEMORY) {
    return ZX_ERR_INVALID_ARGS;
  }

  fbl::RefPtr<JobDispatcher> job;
  zx_status_t status = up->GetDispatcherWithRights(root_job, ZX_RIGHT_MANAGE_PROCESS, &job);
  if (status != ZX_OK) {
    return status;
  }

  // Validate that the job is in fact the first usermode job (aka root job).
  if (GetRootJobDispatcher() != job->parent()) {
    return ZX_ERR_ACCESS_DENIED;
  }

  // Do not grant default event rights, as we don't want userspace to, for
  // example, be able to signal this event.
  return out->make(GetLowMemEvent(), ZX_DEFAULT_SYSTEM_EVENT_LOW_MEMORY_RIGHTS);
}
