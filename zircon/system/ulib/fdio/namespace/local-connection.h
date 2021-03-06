// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <fbl/ref_ptr.h>

#include "../private.h"
#include "local-filesystem.h"
#include "local-vnode.h"

namespace fdio_internal {

// Create an |fdio_t| with a const view of a local node in the namespace.
//
// This object holds strong references to both the local node and local
// filesystem, which are released on |fdio_t|'s close method.
//
// On failure, nullptr is returned.
fdio_t* CreateLocalConnection(fbl::RefPtr<const fdio_namespace> fs,
                              fbl::RefPtr<const LocalVnode> vn);

}  // namespace fdio_internal
