// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdlib.h>

#include <ddk/binding.h>
#include <ddk/debug.h>
#include <ddk/device.h>
#include <ddk/driver.h>
#include <ddk/platform-defs.h>

#define DRIVER_NAME "test-child-2"

typedef struct {
    zx_device_t* zxdev;
} test_t;

static void test_release(void* ctx) {
    free(ctx);
}

static zx_status_t test_rxrpc(void* ctx, zx_handle_t channel) {
    if (channel == ZX_HANDLE_INVALID) {
        return ZX_OK;
    }
    // This won't actually get called, since the other half doesn't send
    // messages at the moment
    __builtin_trap();
};

static zx_protocol_device_t test_device_protocol = {
    .version = DEVICE_OPS_VERSION,
    .rxrpc = test_rxrpc,
    .release = test_release,
};

static zx_status_t test_bind(void* ctx, zx_device_t* parent) {
    zx_status_t status;

    zxlogf(INFO, "test_bind: %s \n", DRIVER_NAME);

    test_t* test = calloc(1, sizeof(test_t));
    if (!test) {
        return ZX_ERR_NO_MEMORY;
    }

    zx_device_prop_t child_props[] = {
        { BIND_PLATFORM_DEV_VID, 0, PDEV_VID_TEST},
        { BIND_PLATFORM_DEV_PID, 0, PDEV_PID_PBUS_TEST},
        { BIND_PLATFORM_DEV_DID, 0, PDEV_DID_TEST_CHILD_4 },
    };

    device_add_args_t args = {
        .version = DEVICE_ADD_ARGS_VERSION,
        .name = "child-4",
        .ctx = test,
        .ops = &test_device_protocol,
        .props = child_props,
        .prop_count = countof(child_props),
        .flags = DEVICE_ADD_MUST_ISOLATE | DEVICE_ADD_ALLOW_MULTI_COMPOSITE,
        .proxy_args = ",",
    };

    status = device_add(parent, &args, &test->zxdev);
    if (status != ZX_OK) {
        zxlogf(ERROR, "%s: device_add failed: %d\n", DRIVER_NAME, status);
        free(test);
        return status;
    }

    return ZX_OK;
}

static zx_driver_ops_t test_driver_ops = {
    .version = DRIVER_OPS_VERSION,
    .bind = test_bind,
};

ZIRCON_DRIVER_BEGIN(test_bus, test_driver_ops, "zircon", "0.1", 3)
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_VID, PDEV_VID_TEST),
    BI_ABORT_IF(NE, BIND_PLATFORM_DEV_PID, PDEV_PID_PBUS_TEST),
    BI_MATCH_IF(EQ, BIND_PLATFORM_DEV_DID, PDEV_DID_TEST_CHILD_2),
ZIRCON_DRIVER_END(test_bus)
