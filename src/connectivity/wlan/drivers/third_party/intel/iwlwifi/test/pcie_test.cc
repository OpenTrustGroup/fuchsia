/*
 * Copyright (c) 2019 The Fuchsia Authors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <lib/fake-bti/bti.h>
#include <lib/mock-function/mock-function.h>
#include <stdio.h>
#include <zircon/listnode.h>

#include <ddk/io-buffer.h>
#include <zxtest/zxtest.h>

extern "C" {
#include "src/connectivity/wlan/drivers/third_party/intel/iwlwifi/pcie/internal.h"
}

namespace {

class PcieTest;

struct iwl_trans_pcie_wrapper {
  struct iwl_trans_pcie trans_pcie;
  PcieTest* test;
};

class PcieTest : public zxtest::Test {
 public:
  PcieTest() {
    trans_ops_.write8 = write8_wrapper;
    trans_ops_.write32 = write32_wrapper;
    trans_ops_.grab_nic_access = grab_nic_access_wrapper;
    trans_ops_.release_nic_access = release_nic_access_wrapper;
    cfg_.base_params = &base_params_;
    trans_ = iwl_trans_alloc(sizeof(struct iwl_trans_pcie_wrapper), &cfg_, &trans_ops_);
    ASSERT_NE(trans_, nullptr);
    auto wrapper = reinterpret_cast<iwl_trans_pcie_wrapper*>(IWL_TRANS_GET_PCIE_TRANS(trans_));
    wrapper->test = this;
    trans_pcie_ = &wrapper->trans_pcie;
    fake_bti_create(&trans_pcie_->bti);
  }

  ~PcieTest() {
    fake_bti_destroy(trans_pcie_->bti);
    iwl_trans_free(trans_);
  }

  mock_function::MockFunction<void, uint32_t, uint8_t> mock_write8_;
  static void write8_wrapper(struct iwl_trans* trans, uint32_t ofs, uint8_t val) {
    auto wrapper = reinterpret_cast<iwl_trans_pcie_wrapper*>(IWL_TRANS_GET_PCIE_TRANS(trans));
    if (wrapper->test->mock_write8_.HasExpectations()) {
      wrapper->test->mock_write8_.Call(ofs, val);
    }
  }

  mock_function::MockFunction<void, uint32_t, uint32_t> mock_write32_;
  static void write32_wrapper(struct iwl_trans* trans, uint32_t ofs, uint32_t val) {
    auto wrapper = reinterpret_cast<iwl_trans_pcie_wrapper*>(IWL_TRANS_GET_PCIE_TRANS(trans));
    if (wrapper->test->mock_write32_.HasExpectations()) {
      wrapper->test->mock_write32_.Call(ofs, val);
    }
  }

  mock_function::MockFunction<bool, unsigned long*> mock_grab_nic_access_;
  static bool grab_nic_access_wrapper(struct iwl_trans* trans, unsigned long* flags) {
    auto wrapper = reinterpret_cast<iwl_trans_pcie_wrapper*>(IWL_TRANS_GET_PCIE_TRANS(trans));
    if (wrapper->test->mock_grab_nic_access_.HasExpectations()) {
      return wrapper->test->mock_grab_nic_access_.Call(flags);
    } else {
      return true;
    }
  }

  mock_function::MockFunction<void, unsigned long*> release_nic_access_;
  static void release_nic_access_wrapper(struct iwl_trans* trans, unsigned long* flags) {
    auto wrapper = reinterpret_cast<iwl_trans_pcie_wrapper*>(IWL_TRANS_GET_PCIE_TRANS(trans));
    if (wrapper->test->release_nic_access_.HasExpectations()) {
      wrapper->test->release_nic_access_.Call(flags);
    }
  }

 protected:
  struct iwl_trans* trans_;
  struct iwl_trans_pcie* trans_pcie_;
  struct iwl_base_params base_params_;
  struct iwl_cfg cfg_;
  struct iwl_trans_ops trans_ops_;
};

TEST_F(PcieTest, DisableInterrupts) {
  mock_write32_.ExpectCall(CSR_INT_MASK, 0x00000000);
  mock_write32_.ExpectCall(CSR_INT, 0xffffffff);
  mock_write32_.ExpectCall(CSR_FH_INT_STATUS, 0xffffffff);

  trans_pcie_->msix_enabled = false;
  set_bit(STATUS_INT_ENABLED, &trans_->status);
  iwl_disable_interrupts(trans_);

  EXPECT_EQ(test_bit(STATUS_INT_ENABLED, &trans_->status), 0);
  mock_write32_.VerifyAndClear();
}

TEST_F(PcieTest, DisableInterruptsMsix) {
  mock_write32_.ExpectCall(CSR_MSIX_FH_INT_MASK_AD, 0xabab);
  mock_write32_.ExpectCall(CSR_MSIX_HW_INT_MASK_AD, 0x4242);

  trans_pcie_->msix_enabled = true;
  trans_pcie_->fh_init_mask = 0xabab;
  trans_pcie_->hw_init_mask = 0x4242;
  set_bit(STATUS_INT_ENABLED, &trans_->status);
  iwl_disable_interrupts(trans_);

  EXPECT_EQ(test_bit(STATUS_INT_ENABLED, &trans_->status), 0);
  mock_write32_.VerifyAndClear();
}

TEST_F(PcieTest, RxInit) {
  trans_->num_rx_queues = 1;
  trans_pcie_->rx_buf_size = IWL_AMSDU_2K;
  ASSERT_OK(iwl_pcie_rx_init(trans_));

  EXPECT_EQ(trans_pcie_->rxq->read, 0);
  EXPECT_EQ(trans_pcie_->rxq->write, 0);
  EXPECT_EQ(trans_pcie_->rxq->free_count, RX_QUEUE_SIZE);
  EXPECT_EQ(trans_pcie_->rxq->used_count, 0);
  EXPECT_EQ(trans_pcie_->rxq->write_actual, 0);
  EXPECT_EQ(trans_pcie_->rxq->queue_size, RX_QUEUE_SIZE);
  EXPECT_GE(list_length(&trans_pcie_->rxq->rx_free), RX_QUEUE_SIZE);
  EXPECT_TRUE(list_is_empty(&trans_pcie_->rxq->rx_used));
  EXPECT_TRUE(io_buffer_is_valid(&trans_pcie_->rxq->rb_status));

  struct iwl_rx_mem_buffer* rxb;
  list_for_every_entry (&trans_pcie_->rxq->rx_free, rxb, struct iwl_rx_mem_buffer, list) {
    EXPECT_TRUE(io_buffer_is_valid(&rxb->io_buf));
    EXPECT_EQ(io_buffer_size(&rxb->io_buf, 0), 2 * 1024);
  }

  iwl_pcie_rx_free(trans_);
}

}  // namespace
