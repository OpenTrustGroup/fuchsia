// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file describes the in-memory structures which construct
// a MinFS filesystem.

#pragma once

#include <errno.h>
#include <inttypes.h>

#include <atomic>

#include <fbl/algorithm.h>
#include <fbl/array.h>
#include <fbl/macros.h>
#include <fbl/unique_ptr.h>
#include <fbl/unique_fd.h>
#include <fs/block-txn.h>
#include <fs/trace.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <minfs/format.h>

#ifdef __Fuchsia__
#include <block-client/cpp/block-device.h>
#include <block-client/cpp/client.h>
#include <fuchsia/hardware/block/c/fidl.h>
#include <fuchsia/hardware/block/volume/c/fidl.h>
#include <fvm/client.h>
#include <lib/fzl/fdio.h>
#include <lib/zx/vmo.h>
#else
#include <fbl/vector.h>
#endif

namespace minfs {

class Bcache : public fs::TransactionHandler {
 public:
  DISALLOW_COPY_ASSIGN_AND_MOVE(Bcache);
  friend class BlockNode;

  ~Bcache() {}

  ////////////////
  // fs::TransactionHandler interface.

  uint32_t FsBlockSize() const final { return kMinfsBlockSize; }

#ifdef __Fuchsia__
  // Acquires a Thread-local group that can be used for sending messages
  // over the block I/O FIFO.
  groupid_t BlockGroupID() final;

  // Return the block size of the underlying block device.
  uint32_t DeviceBlockSize() const final;

  zx_status_t Transaction(block_fifo_request_t* requests, size_t count) final {
    return device_->FifoTransaction(requests, count);
  }
#endif  // __Fuchsia__
  // Raw block read functions.
  // These do not track blocks (or attempt to access the block cache)
  // NOTE: Not marked as final, since these are overridden methods on host,
  // but not on __Fuchsia__.
  zx_status_t Readblk(blk_t bno, void* data);
  zx_status_t Writeblk(blk_t bno, const void* data);

  ////////////////
  // Other methods.

  static zx_status_t Create(fbl::unique_fd fd, uint32_t max_blocks, std::unique_ptr<Bcache>* out);

  // Returns the maximum number of available blocks,
  // assuming the filesystem is non-resizable.
  uint32_t Maxblk() const { return max_blocks_; }

#ifdef __Fuchsia__
  block_client::BlockDevice* device() { return device_.get(); }
  const block_client::BlockDevice* device() const { return device_.get(); }

#else
  // Lengths of each extent (in bytes)
  fbl::Array<size_t> extent_lengths_;
  // Tell Bcache to look for Minfs partition starting at |offset| bytes
  zx_status_t SetOffset(off_t offset);
  // Tell the Bcache it is pointing at a sparse file
  // |offset| indicates where the minfs partition begins within the file
  // |extent_lengths| contains the length of each extent (in bytes)
  zx_status_t SetSparse(off_t offset, const fbl::Vector<size_t>& extent_lengths);
#endif

  int Sync();

 private:
#ifdef __Fuchsia__
  Bcache(fbl::unique_fd fd, std::unique_ptr<block_client::BlockDevice> device, uint32_t max_blocks);
#else
  Bcache(fbl::unique_fd fd, uint32_t max_blocks);
#endif

  const fbl::unique_fd fd_;
  uint32_t max_blocks_;
#ifdef __Fuchsia__
  fuchsia_hardware_block_BlockInfo info_ = {};
  std::atomic<groupid_t> next_group_ = 0;
  std::unique_ptr<block_client::BlockDevice> device_;
#else
  off_t offset_ = 0;
#endif
};

}  // namespace minfs
