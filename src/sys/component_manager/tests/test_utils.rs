// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {
    fdio,
    fidl_fuchsia_sys::FileDescriptor,
    fuchsia_component::client::{self, LaunchOptions},
    fuchsia_runtime::HandleType,
    std::{fs::File, io::Read},
};

pub fn launch_and_wait_for_msg(
    component_to_launch: String,
    args: Option<Vec<String>>,
    expected_msg: String,
) {
    let launcher = client::launcher().expect("failed to connect to launcher");
    let (mut pipe, pipe_handle) = make_pipe();
    let mut options = LaunchOptions::new();
    options.set_out(pipe_handle);
    let _app = client::launch_with_options(&launcher, component_to_launch, args, options)
        .expect("component manager failed to launch");
    read_from_pipe(&mut pipe, expected_msg);
}

fn make_pipe() -> (std::fs::File, FileDescriptor) {
    match fdio::pipe_half() {
        Err(_) => panic!("failed to create pipe"),
        Ok((pipe, handle)) => {
            let pipe_handle = FileDescriptor {
                type0: HandleType::FileDescriptor as i32,
                type1: 0,
                type2: 0,
                handle0: Some(handle.into()),
                handle1: None,
                handle2: None,
            };
            (pipe, pipe_handle)
        }
    }
}

fn read_from_pipe(f: &mut File, expected_msg: String) {
    let mut buf = [0; 1024];

    let mut actual = Vec::new();
    let expected = expected_msg.as_bytes();

    loop {
        let n = f.read(&mut buf).expect("failed to read pipe");
        actual.extend_from_slice(&buf[0..n]);

        if &*actual == expected {
            break;
        }

        if actual.len() >= expected.len() || expected[..actual.len()] != *actual {
            let actual_msg = String::from_utf8(actual.clone())
                .map(|v| format!("'{}'", v))
                .unwrap_or("Non UTF-8".to_string());
            panic!(
                "Received bytes do not match the expectation\n\
                 Expected: '{}'\n\
                 Actual:   {}\n\
                 Actual bytes: {:?}",
                expected_msg, actual_msg, actual
            );
        }
    }
}
