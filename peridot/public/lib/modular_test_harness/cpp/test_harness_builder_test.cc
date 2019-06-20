// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/modular/testing/cpp/fidl.h>
#include <lib/fsl/vmo/strings.h>
#include <lib/modular_test_harness/cpp/fake_component.h>
#include <lib/modular_test_harness/cpp/fake_module.h>
#include <lib/modular_test_harness/cpp/test_harness_builder.h>
#include <lib/modular_test_harness/cpp/test_harness_fixture.h>
#include <lib/sys/cpp/service_directory.h>
#include <lib/sys/cpp/testing/test_with_environment.h>
#include <rapidjson/document.h>
#include <test/modular/test/harness/cpp/fidl.h>

#include "gmock/gmock.h"

using testing::HasSubstr;
using testing::Not;

// TestHarnessBuilder. Provides a service directory, typically used for testing
// environment service building.
class TestHarnessBuilderTest : public modular::testing::TestHarnessFixture {
 public:
  TestHarnessBuilderTest() {
    zx::channel dir_req;
    env_service_dir_ = sys::ServiceDirectory::CreateWithRequest(&dir_req);
    env_pseudo_dir_.Serve(
        fuchsia::io::OPEN_RIGHT_READABLE | fuchsia::io::OPEN_RIGHT_WRITABLE,
        std::move(dir_req));
  }

  template <typename Interface>
  void AddEnvService(fidl::InterfaceRequestHandler<Interface> req_handler) {
    env_pseudo_dir_.AddEntry(
        Interface::Name_,
        std::make_unique<vfs::Service>(
            [req_handler = std::move(req_handler)](
                zx::channel request, async_dispatcher_t* dispatcher) mutable {
              req_handler(
                  fidl::InterfaceRequest<Interface>(std::move(request)));
            }));
  }

  std::shared_ptr<sys::ServiceDirectory> env_service_dir() {
    return env_service_dir_;
  }

 private:
  vfs::PseudoDir env_pseudo_dir_;
  std::shared_ptr<sys::ServiceDirectory> env_service_dir_;
};

// A Pinger implementation used for testing environment services.
class PingerImpl : public test::modular::test::harness::Pinger {
 public:
  bool pinged() { return pinged_; }

 private:
  // |test::modular::test::harness::Pinger|
  void Ping() override { pinged_ = true; }

  bool pinged_ = false;
};

// Test that modular::testing::GenerateFakeUrl() returns new urls each time.
TEST_F(TestHarnessBuilderTest, GenerateFakeUrl) {
  modular::testing::TestHarnessBuilder builder;
  EXPECT_NE(modular::testing::GenerateFakeUrl(),
            modular::testing::GenerateFakeUrl());

  EXPECT_THAT(modular::testing::GenerateFakeUrl("foobar"), HasSubstr("foobar"));
  EXPECT_THAT(modular::testing::GenerateFakeUrl("foo!_bar"),
              HasSubstr("foobar"));
  EXPECT_THAT(modular::testing::GenerateFakeUrl("foo!_bar"),
              Not(HasSubstr("foo!_bar")));
}

bool JsonEq(std::string a, std::string b) {
  rapidjson::Document doc_a;
  doc_a.Parse(a);

  rapidjson::Document doc_b;
  doc_b.Parse(b);

  return doc_a == doc_b;
}

// Test that the TestHarnessBuilder builds a sane TestHarnessSpec and
// OnNewComponent router function.
TEST_F(TestHarnessBuilderTest, InterceptSpecTest) {
  modular::testing::TestHarnessBuilder builder;

  std::string called;
  builder.InterceptComponent(
      [&](auto launch_info, auto handle) { called = "generic"; },
      {.url = "generic", .sandbox_services = {"library.Protocol"}});
  builder.InterceptBaseShell(
      [&](auto launch_info, auto handle) { called = "base_shell"; },
      {.url = "base_shell"});
  builder.InterceptSessionShell(
      [&](auto launch_info, auto handle) { called = "session_shell"; },
      {.url = "session_shell"});
  builder.InterceptStoryShell(
      [&](auto launch_info, auto handle) { called = "story_shell"; },
      {.url = "story_shell"});

  auto spec = builder.BuildSpec();
  EXPECT_EQ("generic", spec.components_to_intercept().at(0).component_url());
  ASSERT_TRUE(spec.components_to_intercept().at(0).has_extra_cmx_contents());
  std::string cmx_str;
  ASSERT_TRUE(fsl::StringFromVmo(
      spec.components_to_intercept().at(0).extra_cmx_contents(), &cmx_str));
  EXPECT_TRUE(
      JsonEq(R"({"sandbox":{"services":["library.Protocol"]}})", cmx_str));
  EXPECT_EQ("base_shell", spec.components_to_intercept().at(1).component_url());
  EXPECT_EQ("session_shell",
            spec.components_to_intercept().at(2).component_url());
  EXPECT_EQ("story_shell",
            spec.components_to_intercept().at(3).component_url());

  EXPECT_EQ("base_shell",
            spec.basemgr_config().base_shell().app_config().url());
  EXPECT_EQ("session_shell", spec.basemgr_config()
                                 .session_shell_map()
                                 .at(0)
                                 .config()
                                 .app_config()
                                 .url());
  EXPECT_EQ("story_shell",
            spec.basemgr_config().story_shell().app_config().url());

  auto handler = builder.BuildOnNewComponentHandler();
  {
    fuchsia::sys::StartupInfo startup_info;
    startup_info.launch_info.url = "generic";
    handler(std::move(startup_info), nullptr);
    EXPECT_EQ("generic", called);
  }
  {
    fuchsia::sys::StartupInfo startup_info;
    startup_info.launch_info.url = "base_shell";
    handler(std::move(startup_info), nullptr);
    EXPECT_EQ("base_shell", called);
  }
  {
    fuchsia::sys::StartupInfo startup_info;
    startup_info.launch_info.url = "session_shell";
    handler(std::move(startup_info), nullptr);
    EXPECT_EQ("session_shell", called);
  }
  {
    fuchsia::sys::StartupInfo startup_info;
    startup_info.launch_info.url = "story_shell";
    handler(std::move(startup_info), nullptr);
    EXPECT_EQ("story_shell", called);
  }
}

// Inject the 'Pinger' service into the env. Test that we can connect to Pinger
// and use it successfully.
TEST_F(TestHarnessBuilderTest, AddService) {
  PingerImpl pinger_impl;
  fidl::BindingSet<test::modular::test::harness::Pinger> pinger_bindings;

  modular::testing::TestHarnessBuilder builder;
  builder.AddService<test::modular::test::harness::Pinger>(
      pinger_bindings.GetHandler(&pinger_impl));
  test_harness().events().OnNewComponent = builder.BuildOnNewComponentHandler();
  test_harness()->Run(builder.BuildSpec());

  test::modular::test::harness::PingerPtr pinger;
  test_harness()->ConnectToEnvironmentService(
      test::modular::test::harness::Pinger::Name_,
      pinger.NewRequest().TakeChannel());

  pinger->Ping();
  RunLoopUntil([&] { return pinger_impl.pinged(); });
}

// Test that TestHarnessBuilder::BuildSpec() populates the
// env_services.services_from_components correctly.
TEST_F(TestHarnessBuilderTest, AddServiceFromComponent) {
  modular::testing::TestHarnessBuilder builder;
  auto fake_url = modular::testing::GenerateFakeUrl();
  builder.AddServiceFromComponent<test::modular::test::harness::Pinger>(
      fake_url);
  auto spec = builder.BuildSpec();

  auto& services_from_components =
      spec.env_services().services_from_components();
  ASSERT_EQ(1u, services_from_components.size());
  EXPECT_EQ(test::modular::test::harness::Pinger::Name_,
            services_from_components[0].name);
  EXPECT_EQ(fake_url, services_from_components[0].url);
}

// Test that InheritService() borrows services from the given |service_dir|.
// This is tested by trying to inherit and use the Pinger service.
TEST_F(TestHarnessBuilderTest, AddServiceFromServiceDirectory) {
  PingerImpl pinger_impl;
  fidl::BindingSet<test::modular::test::harness::Pinger> pinger_bindings;

  AddEnvService<test::modular::test::harness::Pinger>(
      pinger_bindings.GetHandler(&pinger_impl));

  modular::testing::TestHarnessBuilder builder;
  builder.AddServiceFromServiceDirectory<test::modular::test::harness::Pinger>(
      env_service_dir());
  test_harness()->Run(builder.BuildSpec());

  test::modular::test::harness::PingerPtr pinger;
  test_harness()->ConnectToEnvironmentService(
      test::modular::test::harness::Pinger::Name_,
      pinger.NewRequest().TakeChannel());

  pinger->Ping();
  RunLoopUntil([&] { return pinger_impl.pinged(); });
}