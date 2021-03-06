// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <endian.h>
#include <lib/device-protocol/i2c.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <zircon/assert.h>
#include <zircon/hw/i2c.h>
#include <zircon/types.h>

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/protocol/hidbus.h>
#include <ddk/protocol/i2c.h>
#include <ddk/trace/event.h>

#define I2C_HID_DEBUG 0

// Poll interval: 10 ms
#define I2C_POLL_INTERVAL_USEC 10000

#define to_i2c_hid(d) containerof(d, i2c_hid_device_t, hiddev)

typedef struct i2c_hid_desc {
  uint16_t wHIDDescLength;
  uint16_t bcdVersion;
  uint16_t wReportDescLength;
  uint16_t wReportDescRegister;
  uint16_t wInputRegister;
  uint16_t wMaxInputLength;
  uint16_t wOutputRegister;
  uint16_t wMaxOutputLength;
  uint16_t wCommandRegister;
  uint16_t wDataRegister;
  uint16_t wVendorID;
  uint16_t wProductID;
  uint16_t wVersionID;
  uint8_t RESERVED[4];
} __PACKED i2c_hid_desc_t;

typedef struct i2c_hid_device {
  zx_device_t* i2cdev;
  i2c_protocol_t i2c_protocol;

  mtx_t ifc_lock;
  hidbus_ifc_protocol_t ifc;
  void* cookie;

  i2c_hid_desc_t* hiddesc;

  mtx_t i2c_lock;
  cnd_t i2c_reset_cnd;     // Signaled when reset received
  bool i2c_pending_reset;  // True if reset-in-progress
  thrd_t irq_thread;
  zx_handle_t irq;
} i2c_hid_device_t;

// Send the device a HOST initiated RESET.  Caller must call
// i2c_wait_for_ready_locked() afterwards to guarantee completion.
// If |force| is false, do not issue a reset if there is one outstanding.
static zx_status_t i2c_hid_reset(i2c_hid_device_t* dev, bool force) {
  uint16_t cmd_reg = letoh16(dev->hiddesc->wCommandRegister);
  uint8_t buf[4] = {static_cast<uint8_t>(cmd_reg & 0xff), static_cast<uint8_t>(cmd_reg >> 8), 0x00,
                    0x01};

  mtx_lock(&dev->i2c_lock);

  if (!force && dev->i2c_pending_reset) {
    mtx_unlock(&dev->i2c_lock);
    return ZX_OK;
  }

  dev->i2c_pending_reset = true;
  zx_status_t status = i2c_write_sync(&dev->i2c_protocol, buf, sizeof(buf));

  mtx_unlock(&dev->i2c_lock);

  if (status != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: could not issue reset: %d\n", status);
    return status;
  }

  return ZX_OK;
}

// Must be called with i2c_lock held.
static void i2c_wait_for_ready_locked(i2c_hid_device_t* dev) {
  while (dev->i2c_pending_reset) {
    cnd_wait(&dev->i2c_reset_cnd, &dev->i2c_lock);
  }
}

static zx_status_t i2c_hid_query(void* ctx, uint32_t options, hid_info_t* info) {
  if (!info) {
    return ZX_ERR_INVALID_ARGS;
  }
  info->dev_num = 0;
  info->device_class = HID_DEVICE_CLASS_OTHER;
  info->boot_device = false;
  return ZX_OK;
}

static zx_status_t i2c_hid_start(void* ctx, const hidbus_ifc_protocol_t* ifc) {
  i2c_hid_device_t* hid = static_cast<i2c_hid_device_t*>(ctx);
  mtx_lock(&hid->ifc_lock);
  if (hid->ifc.ops) {
    mtx_unlock(&hid->ifc_lock);
    return ZX_ERR_ALREADY_BOUND;
  }
  hid->ifc = *ifc;
  mtx_unlock(&hid->ifc_lock);
  return ZX_OK;
}

static void i2c_hid_stop(void* ctx) {
  i2c_hid_device_t* hid = static_cast<i2c_hid_device_t*>(ctx);
  mtx_lock(&hid->ifc_lock);
  hid->ifc.ops = NULL;
  mtx_unlock(&hid->ifc_lock);
}

static zx_status_t i2c_hid_get_descriptor(void* ctx, uint8_t desc_type, void** data, size_t* len) {
  if (desc_type != HID_DESCRIPTION_TYPE_REPORT) {
    return ZX_ERR_NOT_FOUND;
  }

  i2c_hid_device_t* hid = static_cast<i2c_hid_device_t*>(ctx);
  size_t desc_len = letoh16(hid->hiddesc->wReportDescLength);
  uint16_t desc_reg = letoh16(hid->hiddesc->wReportDescRegister);
  uint16_t buf = htole16(desc_reg);
  uint8_t* out = static_cast<uint8_t*>(malloc(desc_len));
  if (out == NULL) {
    return ZX_ERR_NO_MEMORY;
  }
  i2c_protocol_t i2c;
  zx_status_t status;
  status = device_get_protocol(hid->i2cdev, ZX_PROTOCOL_I2C, &i2c);
  if (status != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: could not get I2C protocol: %d\n", status);
    return ZX_ERR_NOT_SUPPORTED;
  }

  mtx_lock(&hid->i2c_lock);
  i2c_wait_for_ready_locked(hid);

  status = i2c_write_read_sync(&i2c, &buf, sizeof(uint16_t), out, desc_len);
  mtx_unlock(&hid->i2c_lock);
  if (status < 0) {
    zxlogf(ERROR, "i2c-hid: could not read HID report descriptor from reg 0x%04x: %d\n", desc_reg,
           status);
    free(out);
    return ZX_ERR_NOT_SUPPORTED;
  }

  *data = out;
  *len = desc_len;
  return ZX_OK;
}

// TODO: implement the rest of the HID protocol
static zx_status_t i2c_hid_get_report(void* ctx, uint8_t rpt_type, uint8_t rpt_id, void* data,
                                      size_t len, size_t* out_len) {
  return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t i2c_hid_set_report(void* ctx, uint8_t rpt_type, uint8_t rpt_id, const void* data,
                                      size_t len) {
  return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t i2c_hid_get_idle(void* ctx, uint8_t rpt_id, uint8_t* duration) {
  return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t i2c_hid_set_idle(void* ctx, uint8_t rpt_id, uint8_t duration) { return ZX_OK; }

static zx_status_t i2c_hid_get_protocol(void* ctx, uint8_t* protocol) {
  return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t i2c_hid_set_protocol(void* ctx, uint8_t protocol) { return ZX_OK; }

static hidbus_protocol_ops_t i2c_hidbus_ops = []() {
  hidbus_protocol_ops_t i2c_hidbus_ops = {};
  i2c_hidbus_ops.query = i2c_hid_query;
  i2c_hidbus_ops.start = i2c_hid_start;
  i2c_hidbus_ops.stop = i2c_hid_stop;
  i2c_hidbus_ops.get_descriptor = i2c_hid_get_descriptor;
  i2c_hidbus_ops.get_report = i2c_hid_get_report;
  i2c_hidbus_ops.set_report = i2c_hid_set_report;
  i2c_hidbus_ops.get_idle = i2c_hid_get_idle;
  i2c_hidbus_ops.set_idle = i2c_hid_set_idle;
  i2c_hidbus_ops.get_protocol = i2c_hid_get_protocol;
  i2c_hidbus_ops.set_protocol = i2c_hid_set_protocol;
  return i2c_hidbus_ops;
}();

// TODO(teisenbe/tkilbourn): Remove this once we pipe IRQs from ACPI
static int i2c_hid_noirq_thread(void* arg) {
  zxlogf(INFO, "i2c-hid: using noirq\n");

  i2c_hid_device_t* dev = (i2c_hid_device_t*)arg;

  zx_status_t status = i2c_hid_reset(dev, true);
  if (status != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: failed to reset i2c device\n");
    return 0;
  }

  uint16_t len = letoh16(dev->hiddesc->wMaxInputLength);
  uint8_t* buf = static_cast<uint8_t*>(malloc(len));

  // Last report received, so we can deduplicate.  This is only necessary since
  // we haven't wired through interrupts yet, and some devices always return
  // the last received report when you attempt to read from them.
  uint8_t* last_report = static_cast<uint8_t*>(malloc(len));
  size_t last_report_len = 0;

  zx_time_t last_timeout_warning = 0;
  const zx_duration_t kMinTimeBetweenWarnings = ZX_SEC(10);

  // Until we have a way to map the GPIO associated with an i2c slave to an
  // IRQ, we just poll.
  while (true) {
    usleep(I2C_POLL_INTERVAL_USEC);
    TRACE_DURATION("input", "Device Read");
    mtx_lock(&dev->i2c_lock);
    zx_status_t status = i2c_read_sync(&dev->i2c_protocol, buf, len);
    if (status != ZX_OK) {
      if (status == ZX_ERR_TIMED_OUT) {
        zx_time_t now = zx_clock_get_monotonic();
        if (now - last_timeout_warning > kMinTimeBetweenWarnings) {
          zxlogf(TRACE, "i2c-hid: device_read timed out\n");
          last_timeout_warning = now;
        }
        mtx_unlock(&dev->i2c_lock);
        continue;
      }
      zxlogf(ERROR, "i2c-hid: device_read failure %d\n", status);
      mtx_unlock(&dev->i2c_lock);
      continue;
    }

    uint16_t report_len = letoh16(*(uint16_t*)buf);
    if (report_len == 0x0) {
      dev->i2c_pending_reset = false;
      cnd_broadcast(&dev->i2c_reset_cnd);
      mtx_unlock(&dev->i2c_lock);
      continue;
    }
    mtx_unlock(&dev->i2c_lock);

    if (dev->i2c_pending_reset) {
      zxlogf(INFO, "i2c-hid: received event while waiting for reset? %u\n", report_len);
      continue;
    }
    if ((report_len == 0xffff) || (report_len == 0x3fff)) {
      // nothing to read
      continue;
    }
    if (report_len < 2) {
      zxlogf(ERROR, "i2c-hid: bad report len (rlen %hu, bytes read %d)!!!\n", report_len, len);
      continue;
    }

    // Check for duplicates.  See comment by |last_report| definition.
    if (last_report_len == report_len && !memcmp(buf, last_report, report_len)) {
      continue;
    }

    mtx_lock(&dev->ifc_lock);
    if (dev->ifc.ops) {
      hidbus_ifc_io_queue(&dev->ifc, buf + 2, report_len - 2);
    }
    mtx_unlock(&dev->ifc_lock);

    last_report_len = report_len;

    // Swap buffers
    uint8_t* tmp = last_report;
    last_report = buf;
    buf = tmp;
  }

  // TODO: figure out how to clean up
  free(buf);
  free(last_report);
  return 0;
}

static int i2c_hid_irq_thread(void* arg) {
  zxlogf(TRACE, "i2c-hid: using irq\n");

  i2c_hid_device_t* dev = (i2c_hid_device_t*)arg;

  zx_status_t status = i2c_hid_reset(dev, true);
  if (status != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: failed to reset i2c device\n");
    return 0;
  }

  uint16_t len = letoh16(dev->hiddesc->wMaxInputLength);
  uint8_t* buf = static_cast<uint8_t*>(malloc(len));

  zx_time_t last_timeout_warning = 0;
  const zx_duration_t kMinTimeBetweenWarnings = ZX_SEC(10);

  while (true) {
    zx_status_t status = zx_interrupt_wait(dev->irq, NULL);
    if (status != ZX_OK) {
      zxlogf(ERROR, "i2c-hid: interrupt wait failed %d\n", status);
      break;
    }

    TRACE_DURATION("input", "Device Read");
    mtx_lock(&dev->i2c_lock);

    status = i2c_read_sync(&dev->i2c_protocol, buf, len);
    if (status != ZX_OK) {
      if (status == ZX_ERR_TIMED_OUT) {
        zx_time_t now = zx_clock_get_monotonic();
        if (now - last_timeout_warning > kMinTimeBetweenWarnings) {
          zxlogf(TRACE, "i2c-hid: device_read timed out\n");
          last_timeout_warning = now;
        }
        mtx_unlock(&dev->i2c_lock);
        continue;
      }
      zxlogf(ERROR, "i2c-hid: device_read failure %d\n", status);
      mtx_unlock(&dev->i2c_lock);
      continue;
    }

    uint16_t report_len = letoh16(*(uint16_t*)buf);
    if (report_len == 0x0) {
      zxlogf(INFO, "i2c-hid reset detected\n");
      // Either host or device reset.
      dev->i2c_pending_reset = false;
      cnd_broadcast(&dev->i2c_reset_cnd);
      mtx_unlock(&dev->i2c_lock);
      continue;
    }

    if (dev->i2c_pending_reset) {
      zxlogf(INFO, "i2c-hid: received event while waiting for reset? %u\n", report_len);
      mtx_unlock(&dev->i2c_lock);
      continue;
    }

    if (report_len < 2) {
      zxlogf(ERROR, "i2c-hid: bad report len (rlen %hu, bytes read %d)!!!\n", report_len, len);
      mtx_unlock(&dev->i2c_lock);
      continue;
    }

    mtx_unlock(&dev->i2c_lock);

    mtx_lock(&dev->ifc_lock);
    if (dev->ifc.ops) {
      hidbus_ifc_io_queue(&dev->ifc, buf + 2, report_len - 2);
    }
    mtx_unlock(&dev->ifc_lock);
  }

  // TODO: figure out how to clean up
  free(buf);
  return 0;
}

static void i2c_hid_release(void* ctx) { ZX_PANIC("cannot release an i2c hid device yet!\n"); }

static zx_protocol_device_t i2c_hid_dev_ops = []() {
  zx_protocol_device_t i2c_hid_dev_ops = {};
  i2c_hid_dev_ops.version = DEVICE_OPS_VERSION;
  i2c_hid_dev_ops.release = i2c_hid_release;
  return i2c_hid_dev_ops;
}();

static zx_status_t i2c_hid_bind(void* ctx, zx_device_t* dev) {
  zxlogf(TRACE, "i2c_hid_bind\n");

  // Read the i2c HID descriptor
  // TODO: get the address out of ACPI
  uint8_t buf[2];
  uint8_t* data = buf;
  *data++ = 0x01;
  *data++ = 0x00;
  uint8_t out[4];
  i2c_protocol_t i2c;
  zx_status_t status = device_get_protocol(dev, ZX_PROTOCOL_I2C, &i2c);
  if (status != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: could not get I2C protocol: %d\n", status);
    return ZX_ERR_NOT_SUPPORTED;
  }
  if (i2c_write_read_sync(&i2c, buf, sizeof(buf), out, sizeof(out)) != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: could not read HID descriptor: %d\n", status);
    return ZX_ERR_NOT_SUPPORTED;
  }

  i2c_hid_desc_t* i2c_hid_desc_hdr = (i2c_hid_desc_t*)out;
  uint16_t desc_len = letoh16(i2c_hid_desc_hdr->wHIDDescLength);

  i2c_hid_device_t* i2chid = static_cast<i2c_hid_device_t*>(calloc(1, sizeof(i2c_hid_device_t)));
  if (i2chid == NULL) {
    return ZX_ERR_NO_MEMORY;
  }
  mtx_init(&i2chid->ifc_lock, mtx_plain);
  mtx_init(&i2chid->i2c_lock, mtx_plain);
  cnd_init(&i2chid->i2c_reset_cnd);
  i2chid->i2cdev = dev;
  i2chid->i2c_protocol = i2c;
  i2chid->hiddesc = static_cast<i2c_hid_desc_t*>(malloc(desc_len));
  // Mark as pending reset, so no external requests will complete until we
  // reset the device in the IRQ thread.
  i2chid->i2c_pending_reset = true;

  if (i2c_write_read_sync(&i2c, buf, sizeof(buf), i2chid->hiddesc, desc_len) != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: could not read HID descriptor: %d\n", status);
    free(i2chid->hiddesc);
    free(i2chid);
    return ZX_ERR_NOT_SUPPORTED;
  }

  zxlogf(TRACE, "i2c-hid: desc:\n");
  zxlogf(TRACE, "  report desc len: %u\n", letoh16(i2chid->hiddesc->wReportDescLength));
  zxlogf(TRACE, "  report desc reg: %u\n", letoh16(i2chid->hiddesc->wReportDescRegister));
  zxlogf(TRACE, "  input reg:       %u\n", letoh16(i2chid->hiddesc->wInputRegister));
  zxlogf(TRACE, "  max input len:   %u\n", letoh16(i2chid->hiddesc->wMaxInputLength));
  zxlogf(TRACE, "  output reg:      %u\n", letoh16(i2chid->hiddesc->wOutputRegister));
  zxlogf(TRACE, "  max output len:  %u\n", letoh16(i2chid->hiddesc->wMaxOutputLength));
  zxlogf(TRACE, "  command reg:     %u\n", letoh16(i2chid->hiddesc->wCommandRegister));
  zxlogf(TRACE, "  data reg:        %u\n", letoh16(i2chid->hiddesc->wDataRegister));
  zxlogf(TRACE, "  vendor id:       %x\n", i2chid->hiddesc->wVendorID);
  zxlogf(TRACE, "  product id:      %x\n", i2chid->hiddesc->wProductID);
  zxlogf(TRACE, "  version id:      %x\n", i2chid->hiddesc->wVersionID);

  device_add_args_t args = {};
  args.version = DEVICE_ADD_ARGS_VERSION;
  args.name = "i2c-hid";
  args.ctx = i2chid;
  args.ops = &i2c_hid_dev_ops;
  args.proto_id = ZX_PROTOCOL_HIDBUS;
  args.proto_ops = &i2c_hidbus_ops;

  status = device_add(i2chid->i2cdev, &args, NULL);
  if (status != ZX_OK) {
    zxlogf(ERROR, "i2c-hid: could not add device: %d\n", status);
    free(i2chid->hiddesc);
    free(i2chid);
    return status;
  }
  status = i2c_get_interrupt(&i2c, 0, &i2chid->irq);
  int ret;
  if (status != ZX_OK) {
    ret = thrd_create_with_name(&i2chid->irq_thread, i2c_hid_noirq_thread, i2chid, "i2c-hid-noirq");
  } else {
    ret = thrd_create_with_name(&i2chid->irq_thread, i2c_hid_irq_thread, i2chid, "i2c-hid-irq");
  }
  if (ret != thrd_success) {
    zxlogf(ERROR, "i2c-hid: could not create irq thread: %d\n", ret);
    free(i2chid->hiddesc);
    free(i2chid);
    // TODO: map thrd_* status codes to ZX_ERR_* status codes
    return ZX_ERR_INTERNAL;
  }

  return ZX_OK;
}

static zx_driver_ops_t i2c_hid_driver_ops = []() {
  zx_driver_ops_t i2c_hid_driver_ops = {};
  i2c_hid_driver_ops.version = DRIVER_OPS_VERSION;
  i2c_hid_driver_ops.bind = i2c_hid_bind;
  return i2c_hid_driver_ops;
}();

// clang-format off
ZIRCON_DRIVER_BEGIN(i2c_hid, i2c_hid_driver_ops, "zircon", "0.1", 2)
  BI_ABORT_IF(NE, BIND_PROTOCOL, ZX_PROTOCOL_I2C),
  BI_MATCH_IF(EQ, BIND_I2C_CLASS, I2C_CLASS_HID),
ZIRCON_DRIVER_END(i2c_hid)
    // clang-format on
