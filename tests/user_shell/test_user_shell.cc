// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "application/lib/app/connect.h"
#include "application/services/service_provider.fidl.h"
#include "apps/maxwell/services/suggestion/suggestion_provider.fidl.h"
#include "apps/modular/lib/fidl/array_to_string.h"
#include "apps/modular/lib/fidl/single_service_view_app.h"
#include "apps/modular/lib/fidl/view_host.h"
#include "apps/modular/lib/rapidjson/rapidjson.h"
#include "apps/modular/lib/testing/reporting.h"
#include "apps/modular/lib/testing/testing.h"
#include "apps/modular/services/story/link.fidl.h"
#include "apps/modular/services/user/user_context.fidl.h"
#include "apps/modular/services/user/user_shell.fidl.h"
#include "apps/mozart/lib/view_framework/base_view.h"
#include "apps/mozart/services/views/view_manager.fidl.h"
#include "apps/mozart/services/views/view_provider.fidl.h"
#include "apps/test_runner/services/test_runner.fidl.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/functional/make_copyable.h"
#include "lib/ftl/logging.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/tasks/task_runner.h"
#include "lib/ftl/time/time_delta.h"
#include "lib/mtl/tasks/message_loop.h"

using modular::testing::TestPoint;

// TODO(alhaad): Convert this to a proper test which checks the user shell
// interfaces.

namespace {

constexpr char kUserShell[] =
    "https://fuchsia.googlesource.com/modular/"
    "services/user/user_shell.fidl#modular.UserShell";
constexpr char kTestUserShell[] =
    "https://fuchsia.googlesource.com/modular/"
    "tests/user_shell/test_user_shell.cc";

class Settings {
 public:
  explicit Settings(const ftl::CommandLine& command_line) {
    // Good other value to use for dev:
    // "file:///system/apps/example_flutter_counter_parent"
    first_module = command_line.GetOptionValueWithDefault(
        "first_module", "file:///system/apps/example_recipe");
    second_module = command_line.GetOptionValueWithDefault(
        "second_module", "file:///system/apps/example_flutter_hello_world");
    test = command_line.GetOptionValueWithDefault("test", "1") == "1";
  }

  std::string first_module;
  std::string second_module;
  bool test{};
};

class TestUserShellApp : modular::StoryWatcher,
                         modular::StoryProviderWatcher,
                         modular::LinkWatcher,
                         modular::SingleServiceViewApp<modular::UserShell> {
 public:
  explicit TestUserShellApp(const Settings& settings)
      : settings_(settings),
        story_provider_watcher_binding_(this),
        story_watcher_binding_(this),
        link_watcher_binding_(this) {
    if (settings_.test) {
      modular::testing::Init(application_context(), __FILE__);
    }
  }
  ~TestUserShellApp() override = default;

 private:
  // |SingleServiceViewApp|
  void CreateView(
      fidl::InterfaceRequest<mozart::ViewOwner> view_owner_request,
      fidl::InterfaceRequest<app::ServiceProvider> services) override {
    view_.reset(new modular::ViewHost(
        application_context()
            ->ConnectToEnvironmentService<mozart::ViewManager>(),
        std::move(view_owner_request)));
  }

  // |UserShell|
  void Initialize(fidl::InterfaceHandle<modular::UserContext> user_context,
                  fidl::InterfaceHandle<modular::UserShellContext>
                      user_shell_context) override {
    user_context_.Bind(std::move(user_context));

    auto user_shell_context_ptr =
        modular::UserShellContextPtr::Create(std::move(user_shell_context));
    user_shell_context_ptr->GetStoryProvider(story_provider_.NewRequest());
    story_provider_->Watch(story_provider_watcher_binding_.NewBinding());
    story_provider_->GetStoryInfo("X", [](modular::StoryInfoPtr story_info) {
      FTL_LOG(INFO) << "StoryInfo for X is null: " << story_info.is_null();
    });

    user_shell_context_ptr->GetLink(user_shell_link_.NewRequest());
    user_shell_link_->Get(nullptr, [](const fidl::String& value) {
      FTL_LOG(INFO) << "User shell link: " << value;
    });

    story_provider_->PreviousStories([this](fidl::Array<fidl::String> stories) {
      if (stories.size() > 0) {
        std::shared_ptr<unsigned int> count = std::make_shared<unsigned int>(0);
        for (auto& story_id : stories) {
          story_provider_->GetStoryInfo(story_id, [
            this, story_id, count, max = stories.size()
          ](modular::StoryInfoPtr story_info) {
            ++*count;
            if (!story_info.is_null()) {
              FTL_LOG(INFO) << "Previous story " << *count << " of " << max
                            << " " << story_id << " " << story_info->url;
            } else {
              FTL_LOG(INFO) << "Previous story " << *count << " of " << max
                            << " " << story_id << " was deleted";
            }

            if (*count == max) {
              CreateStory(settings_.first_module, true);
            }
          });
        }
      } else {
        CreateStory(settings_.first_module, true);
      }
    });
  }

  // |UserShell|
  void Terminate(const TerminateCallback& done) override {
    FTL_LOG(INFO) << "UserShell::Terminate()";
    // Notify the test runner harness that we can be torn down.
    mtl::MessageLoop::GetCurrent()->PostQuitTask();
    if (settings_.test) {
      modular::testing::Teardown();
    }
    done();
  }

  // |StoryProviderWatcher|
  void OnDelete(const ::fidl::String& story_id) override {
    FTL_VLOG(1) << "TestUserShellApp::OnDelete() " << story_id;
  }

  // |StoryProviderWatcher|
  void OnChange(modular::StoryInfoPtr story_info) override {
    FTL_VLOG(1) << "TestUserShellApp::OnChange() "
                << " id " << story_info->id << " is_running "
                << story_info->is_running << " state " << story_info->state
                << " url " << story_info->url;
  }

  // |StoryWatcher|
  void OnStateChange(modular::StoryState state) override {
    if (state != modular::StoryState::DONE) {
      return;
    }

    FTL_LOG(INFO) << "TestUserShell DONE";
    story_controller_->Stop([this] {
      TearDownStoryController();

      // When the story is done, we start the next one.
      mtl::MessageLoop::GetCurrent()->task_runner()->PostDelayedTask(
          [this] { CreateStory(settings_.second_module, false); },
          ftl::TimeDelta::FromSeconds(20));
    });
  }

  // |LinkWatcher|
  void Notify(const fidl::String& json) override {
    if (++data_count_ % 5 == 0) {
      StopExampleStory();
    }
  }

  void CreateStory(const fidl::String& url, const bool keep) {
    modular::JsonDoc doc;
    std::vector<std::string> segments{"example", url, "created-with-info"};
    modular::CreatePointer(doc, segments.begin(), segments.end())
        .Set(doc, true);

    using FidlStringMap = fidl::Map<fidl::String, fidl::String>;
    story_provider_->CreateStoryWithInfo(
        url, FidlStringMap(), modular::JsonValueToString(doc),
        [this, keep](const fidl::String& story_id) {
          GetStoryInfo(story_id, keep);
        });
  }

  void GetModules() {
    FTL_DCHECK(story_controller_);

    story_controller_->GetModules([this](fidl::Array<modular::ModuleDataPtr> modules) {
      if (settings_.test) {
        get_modules_.Pass();
      }

      FTL_LOG(INFO) << "TestUserShell MODULES:";
      for (const auto& module_data : modules) {
        FTL_LOG(INFO) << "TestUserShell MODULE: url=" << module_data->url;
        FTL_LOG(INFO) << "TestUserShell         link=" << module_data->link;
        std::string path;
        for (const auto& path_element : module_data->module_path) {
          path.push_back(' ');
          path.append(path_element);
        }
        if (path.size()) {
          FTL_LOG(INFO) << "TestUserShell         path=" << path.substr(1);
        }
      }
    });

  }

  void GetStoryInfo(const fidl::String& story_id, const bool keep) {
    story_provider_->GetController(story_id, story_controller_.NewRequest());
    story_controller_->GetInfo([this, keep](modular::StoryInfoPtr story_info) {
      story_info_ = std::move(story_info);
      FTL_LOG(INFO) << "TestUserShell START " << story_info_->id << " "
                    << story_info_->url;
      InitStory();

      if (!keep) {
        GetModules();
        mtl::MessageLoop::GetCurrent()->task_runner()->PostDelayedTask(
            [this] {
              FTL_LOG(INFO) << "TestUserShell DELETE " << story_info_->id;
              story_provider_->DeleteStory(story_info_->id, [this]() {
                FTL_LOG(INFO) << "TestUserShell DELETE STORY DONE";
                user_context_->Logout();
              });
            },
            ftl::TimeDelta::FromSeconds(20));
      }
    });
  }

  void GetController() {
    FTL_LOG(INFO) << "TestUserShell RESUME";
    story_provider_->GetController(story_info_->id,
                                   story_controller_.NewRequest());
    InitStory();
  }

  void InitStory() {
    story_controller_->GetLink(root_.NewRequest());

    // NOTE(mesch): Totally tentative. Tell the root module under what
    // user shell it's running.
    std::vector<std::string> segments{"startup", "stories",
                                      story_info_->url.get(), kUserShell};
    root_->Set(fidl::Array<fidl::String>::From(segments),
               modular::JsonValueToString(modular::JsonValue(kTestUserShell)));

    // NOTE(mesch): Both watchers below fire right after they are
    // registered. Make sure the link data watcher doesn't stop us
    // right away.
    data_count_ = 0;

    story_controller_->Watch(story_watcher_binding_.NewBinding());
    root_->Watch(link_watcher_binding_.NewBinding());

    fidl::InterfaceHandle<mozart::ViewOwner> story_view;
    story_controller_->Start(story_view.NewRequest());

    // Show the new story.
    view_->ConnectView(std::move(story_view));

    story_controller_->AddModule("second", settings_.second_module, "root2");
  }

  // Every five counter increments, we dehydrate and rehydrate the story.
  void StopExampleStory() {
    FTL_LOG(INFO) << "TestUserShell STOP";

    story_provider_->GetStoryInfo(
        story_info_->id, [this](modular::StoryInfoPtr story_info) {
          FTL_LOG(INFO) << "TestUserShell STOP got story info";
          FTL_DCHECK(!story_info.is_null());
          FTL_DCHECK(story_info->is_running == true);
          story_controller_->Stop([this] {
            FTL_LOG(INFO) << "TestUserShell STOP done";
            TearDownStoryController();

            // When the story stops, we start it again.
            mtl::MessageLoop::GetCurrent()->task_runner()->PostDelayedTask(
                [this] {
                  story_provider_->GetStoryInfo(
                      story_info_->id,
                      [this](modular::StoryInfoPtr story_info) {
                        FTL_DCHECK(!story_info.is_null());
                        FTL_DCHECK(story_info->is_running == false);
                        GetController();
                      });
                },
                ftl::TimeDelta::FromSeconds(10));
          });
        });
  }

  void TearDownStoryController() {
    story_watcher_binding_.Close();
    link_watcher_binding_.Close();
    story_controller_.reset();
    root_.reset();
  }

  const Settings settings_;
  fidl::Binding<modular::StoryProviderWatcher> story_provider_watcher_binding_;
  fidl::Binding<modular::StoryWatcher> story_watcher_binding_;
  fidl::Binding<modular::LinkWatcher> link_watcher_binding_;
  std::unique_ptr<modular::ViewHost> view_;
  modular::UserContextPtr user_context_;
  modular::StoryProviderPtr story_provider_;
  modular::StoryControllerPtr story_controller_;
  modular::LinkPtr root_;
  modular::LinkPtr user_shell_link_;
  modular::StoryInfoPtr story_info_;
  int data_count_{0};

  TestPoint get_modules_{"Get Modules"};

  FTL_DISALLOW_COPY_AND_ASSIGN(TestUserShellApp);
};

}  // namespace

int main(int argc, const char** argv) {
  auto command_line = ftl::CommandLineFromArgcArgv(argc, argv);
  Settings settings(command_line);

  mtl::MessageLoop loop;
  TestUserShellApp app(settings);
  loop.Run();
  return 0;
}
