// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <lib/cmdline.h>
#include <lib/userabi/vdso-constants.h>
#include <lib/userabi/vdso.h>
#include <platform.h>
#include <zircon/types.h>

#include <fbl/alloc_checker.h>
#include <object/handle.h>
#include <vm/pmm.h>
#include <vm/vm.h>
#include <vm/vm_aspace.h>
#include <vm/vm_object.h>

#include "config-buildid.h"
#include "vdso-code.h"

// This is defined in assembly via RODSO_IMAGE (see rodso-asm.h);
// vdso-code.h gives details about the image's size and layout.
extern "C" const char vdso_image[];

namespace {

// Each KernelVmoWindow object represents a mapping in the kernel address
// space of a T object found inside a VM object.  The kernel mapping exists
// for the lifetime of the KernelVmoWindow object.
template <typename T>
class KernelVmoWindow {
 public:
  static_assert(__is_pod(T), "this is for C-compatible types only!");

  KernelVmoWindow(const char* name, fbl::RefPtr<VmObject> vmo, uint64_t offset)
      : mapping_(nullptr) {
    uint64_t page_offset = ROUNDDOWN(offset, PAGE_SIZE);
    size_t offset_in_page = static_cast<size_t>(offset % PAGE_SIZE);
    ASSERT(offset % alignof(T) == 0);

    const size_t size = offset_in_page + sizeof(T);
    const uint arch_mmu_flags = ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE;
    zx_status_t status = VmAspace::kernel_aspace()->RootVmar()->CreateVmMapping(
        0 /* ignored */, size, 0 /* align pow2 */, 0 /* vmar flags */, ktl::move(vmo), page_offset,
        arch_mmu_flags, name, &mapping_);
    ASSERT(status == ZX_OK);
    data_ = reinterpret_cast<T*>(mapping_->base() + offset_in_page);
  }

  ~KernelVmoWindow() {
    if (mapping_) {
      zx_status_t status = mapping_->Destroy();
      ASSERT(status == ZX_OK);
    }
  }

  T* data() const { return data_; }

 private:
  fbl::RefPtr<VmMapping> mapping_;
  T* data_;
};

// The .dynsym section of the vDSO, an array of ELF symbol table entries.
struct VDsoDynSym {
  struct {
    uintptr_t info, value, size;
  } table[VDSO_DYNSYM_COUNT];
};

#define PASTE(a, b, c) PASTE_1(a, b, c)
#define PASTE_1(a, b, c) a##b##c

class VDsoDynSymWindow {
 public:
  DISALLOW_COPY_ASSIGN_AND_MOVE(VDsoDynSymWindow);

  static_assert(sizeof(VDsoDynSym) == VDSO_DATA_END_dynsym - VDSO_DATA_START_dynsym,
                "either VDsoDynsym or gen-rodso-code.sh is suspect");

  explicit VDsoDynSymWindow(fbl::RefPtr<VmObject> vmo)
      : window_("vDSO .dynsym", ktl::move(vmo), VDSO_DATA_START_dynsym) {}

  void get_symbol_entry(size_t i, uintptr_t* value, size_t* size) {
    *value = window_.data()->table[i].value;
    *size = window_.data()->table[i].size;
  }

  void set_symbol_entry(size_t i, uintptr_t value, size_t size) {
    window_.data()->table[i].value = value;
    window_.data()->table[i].size = size;
  }

  void localize_symbol_entry(size_t i) {
    // The high nybble is the STB_* bits; STB_LOCAL is 0.
    window_.data()->table[i].info &= 0xf;
  }

#define get_symbol(symbol, value, size) get_symbol_entry(PASTE(VDSO_DYNSYM_, symbol, ), value, size)

#define set_symbol(symbol, target)                                             \
  set_symbol_entry(PASTE(VDSO_DYNSYM_, symbol, ), PASTE(VDSO_CODE_, target, ), \
                   PASTE(VDSO_CODE_, target, _SIZE))

#define localize_symbol(symbol) localize_symbol_entry(PASTE(VDSO_DYNSYM_, symbol, ))

 private:
  KernelVmoWindow<VDsoDynSym> window_;
};

class VDsoCodeWindow {
 public:
  DISALLOW_COPY_ASSIGN_AND_MOVE(VDsoCodeWindow);

  using CodeBuffer = uint8_t[VDSO_CODE_END - VDSO_CODE_START];

  explicit VDsoCodeWindow(fbl::RefPtr<VmObject> vmo)
      : window_("vDSO code segment", ktl::move(vmo), VDSO_CODE_START) {}

  // Fill the given code region (a whole function) with safely invalid code.
  // This code should never be run, and any attempt to use it should crash.
  void block_execution(uintptr_t address, size_t size) {
    ASSERT(address >= VDSO_CODE_START);
    ASSERT(address + size < VDSO_CODE_END);
    address -= VDSO_CODE_START;

#if ARCH_X86

    // Fill with the single-byte HLT instruction, so any place
    // user-mode jumps into this code, it gets a trap.
    memset(&Code()[address], 0xf4, size);

#elif ARCH_ARM64

    // Fixed-size instructions.
    ASSERT(address % 4 == 0);
    ASSERT(size % 4 == 0);
    uint32_t* code = reinterpret_cast<uint32_t*>(&Code()[address]);
    for (size_t i = 0; i < size / 4; ++i)
      code[i] = 0xd4200020;  // 'brk #1' (what __builtin_trap() emits)

#else
#error what architecture?
#endif
  }

 private:
  CodeBuffer& Code() { return *window_.data(); }

  KernelVmoWindow<CodeBuffer> window_;
};

#define REDIRECT_SYSCALL(dynsym_window, symbol, target) \
  do {                                                  \
    dynsym_window.set_symbol(symbol, target);           \
    dynsym_window.set_symbol(_##symbol, target);        \
  } while (0)

// Block the named zx_* function.  The symbol table entry will
// become invisible to runtime symbol resolution, and the code of
// the function will be clobbered with trapping instructions.
#define BLOCK_SYSCALL(dynsym_window, code_window, symbol)   \
  do {                                                      \
    dynsym_window.localize_symbol(symbol);                  \
    dynsym_window.localize_symbol(_##symbol);               \
    uintptr_t address, _address;                            \
    size_t size, _size;                                     \
    dynsym_window.get_symbol(symbol, &address, &size);      \
    dynsym_window.get_symbol(_##symbol, &_address, &_size); \
    ASSERT(address == _address);                            \
    ASSERT(size == _size);                                  \
    code_window.block_execution(address, size);             \
  } while (0)

// Random attributes in syscalls.abigen become "categories" of syscalls.
// For each category, define a function block_<category> to block all the
// syscalls in that category.  These functions can be used in
// VDso::CreateVariant (below) to block a category of syscalls for a particular
// variant vDSO.
#define SYSCALL_CATEGORY_BEGIN(category) \
  void block_##category##_syscalls(VDsoDynSymWindow& dynsym_window, VDsoCodeWindow& code_window) {
#define SYSCALL_IN_CATEGORY(syscall) BLOCK_SYSCALL(dynsym_window, code_window, zx_##syscall);
#define SYSCALL_CATEGORY_END(category) }
#include <zircon/syscall-category.inc>
#undef SYSCALL_CATEGORY_BEGIN
#undef SYSCALL_IN_CATEGORY_END
#undef SYSCALL_CATEGORY_END

}  // anonymous namespace

const VDso* VDso::instance_ = NULL;

// Private constructor, can only be called by Create (below).
VDso::VDso() : RoDso("vdso/full", vdso_image, VDSO_CODE_END, VDSO_CODE_START) {}

// This is called exactly once, at boot time.
const VDso* VDso::Create() {
  ASSERT(!instance_);

  fbl::AllocChecker ac;
  VDso* vdso = new (&ac) VDso();
  ASSERT(ac.check());

  // Map a window into the VMO to write the vdso_constants struct.
  static_assert(sizeof(vdso_constants) == VDSO_DATA_CONSTANTS_SIZE, "gen-rodso-code.sh is suspect");
  KernelVmoWindow<vdso_constants> constants_window("vDSO constants", vdso->vmo()->vmo(),
                                                   VDSO_DATA_CONSTANTS);
  zx_ticks_t per_second = ticks_per_second();

  // Initialize the constants that should be visible to the vDSO.
  // Rather than assigning each member individually, do this with
  // struct assignment and a compound literal so that the compiler
  // can warn if the initializer list omits any member.
  *constants_window.data() = (vdso_constants){
      arch_max_num_cpus(),
      {
          arch_cpu_features(),
          arch_get_hw_breakpoint_count(),
          arch_get_hw_watchpoint_count(),
      },
      arch_dcache_line_size(),
      arch_icache_line_size(),
      per_second,
      pmm_count_total_bytes(),
      BUILDID,
  };

  // If ticks_per_second has not been calibrated, it will return 0. In this
  // case, use soft_ticks instead.
  if (per_second == 0 || gCmdline.GetBool("vdso.soft_ticks", false)) {
    // Make zx_ticks_per_second return nanoseconds per second.
    constants_window.data()->ticks_per_second = ZX_SEC(1);

    // Adjust the zx_ticks_get entry point to be soft_ticks_get.
    VDsoDynSymWindow dynsym_window(vdso->vmo()->vmo());
    REDIRECT_SYSCALL(dynsym_window, zx_ticks_get, soft_ticks_get);
  }

  for (size_t v = static_cast<size_t>(Variant::FULL) + 1; v < static_cast<size_t>(Variant::COUNT);
       ++v)
    vdso->CreateVariant(static_cast<Variant>(v));

  instance_ = vdso;
  return instance_;
}

uintptr_t VDso::base_address(const fbl::RefPtr<VmMapping>& code_mapping) {
  return code_mapping ? code_mapping->base() - VDSO_CODE_START : 0;
}

void VDso::GetVariants(Handle** vmos) const {
  DEBUG_ASSERT(!(vmo_rights() & ZX_RIGHT_WRITE));
  vmos[0] = RoDso::vmo_handle().release();
  for (size_t i = 1; i < static_cast<size_t>(Variant::COUNT); ++i) {
    vmos[i] = Handle::Make(variant_vmo_[i - 1], vmo_rights()).release();
  }
}

// Each vDSO variant VMO is made via a COW clone of the main/default vDSO
// VMO.  A variant can block some system calls, by syscall category.
// This works by modifying the symbol table entries to make the symbols
// invisible to dynamic linking (STB_LOCAL) and then clobbering the code
// with trapping instructions.  In this way, all the code locations are the
// same across variants and the syscall entry enforcement doesn't have to
// care which variant is in use.  The places where the blocked
// syscalls' syscall entry instructions would be no longer have the syscall
// instructions, so a process using the variant can never get into syscall
// entry with that PC value and hence can never pass the vDSO enforcement
// test.
void VDso::CreateVariant(Variant variant) {
  DEBUG_ASSERT(variant > Variant::FULL);
  DEBUG_ASSERT(variant < Variant::COUNT);
  DEBUG_ASSERT(!variant_vmo_[variant_index(variant)]);

  fbl::RefPtr<VmObject> new_vmo;
  zx_status_t status = vmo()->CreateChild(ZX_VMO_CHILD_COPY_ON_WRITE, 0, size(), false, &new_vmo);
  ASSERT(status == ZX_OK);

  VDsoDynSymWindow dynsym_window(new_vmo);
  VDsoCodeWindow code_window(new_vmo);

  const char* name = nullptr;
  switch (variant) {
    case Variant::TEST1:
      name = "vdso/test1";
      block_test_category1_syscalls(dynsym_window, code_window);
      break;

    case Variant::TEST2:
      name = "vdso/test2";
      block_test_category2_syscalls(dynsym_window, code_window);
      break;

    // No default case so the compiler will warn about new enum entries.
    case Variant::FULL:
    case Variant::COUNT:
      PANIC("VDso::CreateVariant called with bad variant");
  }

  fbl::RefPtr<Dispatcher> dispatcher;
  zx_rights_t rights;
  status = VmObjectDispatcher::Create(ktl::move(new_vmo), &dispatcher, &rights);
  ASSERT(status == ZX_OK);

  status = dispatcher->set_name(name, strlen(name));
  ASSERT(status == ZX_OK);

  variant_vmo_[variant_index(variant)] = DownCastDispatcher<VmObjectDispatcher>(&dispatcher);
}
