// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GARNET_BIN_TRACE_MANAGER_APP_H_
#define GARNET_BIN_TRACE_MANAGER_APP_H_

#include <fuchsia/tracing/controller/cpp/fidl.h>
#include <fuchsia/tracing/provider/cpp/fidl.h>
#include <lib/fidl/cpp/binding_set.h>
#include <lib/sys/cpp/component_context.h>

#include <memory>

#include "garnet/bin/trace_manager/trace_manager.h"

namespace tracing {

class TraceManagerApp {
 public:
  explicit TraceManagerApp(const Config& config);
  ~TraceManagerApp();

 private:
  std::unique_ptr<sys::ComponentContext> context_;
  TraceManager trace_manager_;
  fidl::BindingSet<fuchsia::tracing::provider::Registry> trace_registry_bindings_;
  fidl::BindingSet<fuchsia::tracing::controller::Controller> trace_controller_bindings_;

  TraceManagerApp(const TraceManagerApp&) = delete;
  TraceManagerApp(TraceManagerApp&&) = delete;
  TraceManagerApp& operator=(const TraceManagerApp&) = delete;
  TraceManagerApp& operator=(TraceManagerApp&&) = delete;
};

}  // namespace tracing

#endif  // GARNET_BIN_TRACE_MANAGER_APP_H_
