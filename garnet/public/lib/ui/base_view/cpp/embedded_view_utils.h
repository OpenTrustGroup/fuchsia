// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIB_UI_BASE_VIEW_CPP_EMBEDDED_VIEW_UTILS_H_
#define LIB_UI_BASE_VIEW_CPP_EMBEDDED_VIEW_UTILS_H_

#include <fuchsia/ui/app/cpp/fidl.h>
#include <fuchsia/ui/gfx/cpp/fidl.h>
#include <fuchsia/ui/input/cpp/fidl.h>
#include <fuchsia/ui/scenic/cpp/fidl.h>
#include <fuchsia/ui/views/cpp/fidl.h>

#include "lib/component/cpp/startup_context.h"
#include "lib/svc/cpp/services.h"
#include "lib/ui/scenic/cpp/resources.h"
#include "lib/ui/scenic/cpp/session.h"

namespace scenic {

// Serves as the return value for LaunchAppAndCreateView(), below.
struct EmbeddedViewInfo {
  // Controls the launched app.  The app will be destroyed if this connection
  // is closed.
  fuchsia::sys::ComponentControllerPtr controller;

  // Services provided by the launched app.  Must not be destroyed
  // immediately, otherwise the |view_provider| connection may not be
  // established.
  component::Services app_services;

  // ViewProvider service obtained from the app via |app_services|.  Must not
  // be destroyed immediately, otherwise the call to CreateView() might not be
  // processed.
  fuchsia::ui::app::ViewProviderPtr view_provider;

  // A token that can be used to create a ViewHolder; the corresponding token
  // was provided to |view_provider| via ViewProvider.CreateView().  The
  // launched app is expected to create a View, which will be connected to the
  // ViewHolder created with this token.
  fuchsia::ui::views::ViewHolderToken view_holder_token;

  // Handle to services provided by ViewProvider.CreateView().
  fidl::InterfaceHandle<fuchsia::sys::ServiceProvider> services_from_child_view;

  // Interface request for services provided to ViewProvider.CreateView(); the
  // caller of LaunchAppAndCreateView() may choose to attach this request to a
  // ServiceProvider implementation.
  fidl::InterfaceRequest<fuchsia::sys::ServiceProvider> services_to_child_view;
};

// Launch a component and connect to its ViewProvider service, passing it the
// necessary information to attach itself as a child view.  Populates the
// returned EmbeddedViewInfo, which the caller can use to embed the child.
// For example, an interface to a ViewProvider is obtained, a pair of
// zx::eventpairs is created, CreateView is called, etc.  This encapsulates
// the boilerplate the the client would otherwise write themselves.
EmbeddedViewInfo LaunchComponentAndCreateView(const fuchsia::sys::LauncherPtr& launcher,
                                              const std::string& component_url,
                                              const std::vector<std::string>& component_args = {});

}  // namespace scenic

#endif  // LIB_UI_BASE_VIEW_CPP_EMBEDDED_VIEW_UTILS_H_
