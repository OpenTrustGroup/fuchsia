// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <inttypes.h>
#include <lib/userabi/rodso.h>

#include <object/handle.h>
#include <object/vm_address_region_dispatcher.h>
#include <object/vm_object_dispatcher.h>
#include <vm/vm_address_region.h>
#include <vm/vm_aspace.h>
#include <vm/vm_object.h>
#include <vm/vm_object_paged.h>

EmbeddedVmo::EmbeddedVmo(const char* name, const void* image, size_t size)
    : name_(name), size_(size) {
  DEBUG_ASSERT(IS_PAGE_ALIGNED(size));
  fbl::RefPtr<Dispatcher> dispatcher;

  // create vmo out of ro data mapped in kernel space
  fbl::RefPtr<VmObject> vmo;
  zx_status_t status = VmObjectPaged::CreateFromWiredPages(image, size, true, &vmo);
  ASSERT(status == ZX_OK);

  // build and point a dispatcher at it
  status = VmObjectDispatcher::Create(ktl::move(vmo), &dispatcher, &vmo_rights_);
  ASSERT(status == ZX_OK);

  status = dispatcher->set_name(name, strlen(name));
  ASSERT(status == ZX_OK);
  vmo_ = DownCastDispatcher<VmObjectDispatcher>(&dispatcher);
  vmo_rights_ &= ~ZX_RIGHT_WRITE;
  vmo_rights_ |= ZX_RIGHT_EXECUTE;
}

HandleOwner EmbeddedVmo::vmo_handle() const { return Handle::Make(vmo_, vmo_rights_); }

// Map one segment from our VM object.
zx_status_t EmbeddedVmo::MapSegment(fbl::RefPtr<VmAddressRegionDispatcher> vmar, bool code,
                                    size_t vmar_offset, size_t start_offset,
                                    size_t end_offset) const {
  uint32_t flags = ZX_VM_SPECIFIC | ZX_VM_PERM_READ;
  if (code)
    flags |= ZX_VM_PERM_EXECUTE;

  size_t len = end_offset - start_offset;

  fbl::RefPtr<VmMapping> mapping;
  zx_status_t status = vmar->Map(vmar_offset, vmo_->vmo(), start_offset, len, flags, &mapping);

  const char* segment_name = code ? "code" : "rodata";
  if (status != ZX_OK) {
    dprintf(CRITICAL, "userboot: %s %s mapping %#zx @ %#" PRIxPTR " size %#zx failed %d\n", name_,
            segment_name, start_offset, vmar->vmar()->base() + vmar_offset, len, status);
  } else {
    DEBUG_ASSERT(mapping->base() == vmar->vmar()->base() + vmar_offset);
    dprintf(SPEW, "userboot: %-8s %-6s %#7zx @ [%#" PRIxPTR ",%#" PRIxPTR ")\n", name_,
            segment_name, start_offset, mapping->base(), mapping->base() + len);
  }

  return status;
}

zx_status_t RoDso::Map(fbl::RefPtr<VmAddressRegionDispatcher> vmar, size_t offset) const {
  zx_status_t status = MapSegment(vmar, false, offset, 0, code_start_);
  if (status == ZX_OK)
    status = MapSegment(ktl::move(vmar), true, offset + code_start_, code_start_, size());
  return status;
}
