// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_CAMERA_SIMPLE_CAMERA_SIMPLE_CAMERA_SERVER_SIMPLE_CAMERA_SERVER_APP_H_
#define SRC_CAMERA_SIMPLE_CAMERA_SIMPLE_CAMERA_SERVER_SIMPLE_CAMERA_SERVER_APP_H_

#include <fuchsia/images/cpp/fidl.h>
#include <fuchsia/simplecamera/cpp/fidl.h>

#include <src/camera/simple_camera/simple_camera_lib/video_display.h>

#include "lib/component/cpp/startup_context.h"

namespace simple_camera {

class SimpleCameraApp : public fuchsia::simplecamera::SimpleCamera {
 public:
  SimpleCameraApp();
  virtual void ConnectToCamera(uint32_t camera_id,
                               fidl::InterfaceHandle<fuchsia::images::ImagePipe> image_pipe);

 private:
  SimpleCameraApp(const SimpleCameraApp&) = delete;
  SimpleCameraApp& operator=(const SimpleCameraApp&) = delete;

  std::unique_ptr<component::StartupContext> context_;
  fidl::BindingSet<SimpleCamera> bindings_;

  simple_camera::VideoDisplay video_display_;
};

}  // namespace simple_camera

#endif  // SRC_CAMERA_SIMPLE_CAMERA_SIMPLE_CAMERA_SERVER_SIMPLE_CAMERA_SERVER_APP_H_
