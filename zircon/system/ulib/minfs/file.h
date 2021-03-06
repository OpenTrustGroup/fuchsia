// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#ifdef __Fuchsia__
#include "vnode-allocation.h"
#endif

#include <fbl/algorithm.h>
#include <fbl/ref_ptr.h>
#include <fbl/unique_ptr.h>
#include <fs/block-txn.h>
#include <fs/trace.h>
#include <fs/vnode.h>
#include <lib/zircon-internal/fnv1hash.h>
#include <minfs/format.h>
#include <minfs/minfs.h>
#include <minfs/superblock.h>
#include <minfs/transaction-limits.h>
#include <minfs/writeback.h>

#include "vnode.h"

namespace minfs {

// A specialization of the Minfs Vnode which implements a regular file interface.
class File final : public VnodeMinfs, public fbl::Recyclable<File> {
 public:
  File(Minfs* fs);
  ~File();

  // fbl::Recyclable interface.
  void fbl_recycle() final { VnodeMinfs::fbl_recycle(); }

 private:
  bool IsDirectory() const final { return false; }
  zx_status_t CanUnlink() const final;

  // minfs::Vnode interface.
  blk_t GetBlockCount() const final;
  uint64_t GetSize() const final;
  void SetSize(uint32_t new_size) final;
  void AcquireWritableBlock(Transaction* transaction, blk_t local_bno, blk_t old_bno,
                            blk_t* out_bno) final;
  void DeleteBlock(Transaction* transaction, blk_t local_bno, blk_t old_bno) final;
#ifdef __Fuchsia__
  void IssueWriteback(Transaction* transaction, blk_t vmo_offset, blk_t dev_offset,
                      blk_t count) final;
  bool HasPendingAllocation(blk_t vmo_offset) final;
  void CancelPendingWriteback() final;
#endif

  // fs::Vnode interface.
  zx_status_t ValidateFlags(uint32_t flags) final;
  zx_status_t Read(void* data, size_t len, size_t off, size_t* out_actual) final;
  zx_status_t Write(const void* data, size_t len, size_t offset, size_t* out_actual) final;
  zx_status_t Append(const void* data, size_t len, size_t* out_end, size_t* out_actual) final;
  zx_status_t Truncate(size_t len) final;

#ifdef __Fuchsia__
  // Allocate all data blocks pending in |allocation_state_|.
  void AllocateData();

  // For all data blocks in the range |start| to |start + count|, reserve specific blocks in
  // the allocator to be swapped in at the time the old blocks are swapped out. Metadata blocks
  // are expected to have been allocated previously.
  zx_status_t BlocksSwap(Transaction* state, blk_t start, blk_t count, blk_t* bno);

  // Describes pending allocation data for the vnode. This should only be accessed while a valid
  // Transaction object is held, as it may be modified asynchronously by the DataBlockAssigner
  // thread.
  PendingAllocationData allocation_state_;
#endif
};

}  // namespace minfs
