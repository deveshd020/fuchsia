// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fuchsia/ui/policy/cpp/fidl.h>
#include <lib/async-loop/cpp/loop.h>
#include <lib/async-loop/default.h>
#include <lib/async/cpp/task.h>
#include <lib/sys/cpp/component_context.h>
#include <lib/zx/channel.h>

#include "src/lib/fxl/command_line.h"
#include "src/lib/fxl/log_settings_command_line.h"
#include "src/lib/fxl/logging.h"
#include "src/ui/bin/root_presenter/renderer_params.h"

int main(int argc, const char** argv) {
  auto command_line = fxl::CommandLineFromArgcArgv(argc, argv);
  if (!fxl::SetLogSettingsFromCommandLine(command_line))
    return 1;

  FXL_LOG(WARNING) << "This tool is intended for testing and debugging purposes "
                      "only and may cause problems if invoked incorrectly.";

  root_presenter::RendererParams presenter_renderer_params =
      root_presenter::RendererParams::FromCommandLine(command_line);

  bool clipping_enabled = true;
  if (presenter_renderer_params.clipping_enabled.has_value()) {
    clipping_enabled = presenter_renderer_params.clipping_enabled.value();
  }

  std::vector<fuchsia::ui::gfx::RendererParam> renderer_params;
  if (presenter_renderer_params.render_frequency.has_value()) {
    fuchsia::ui::gfx::RendererParam param;
    param.set_render_frequency(presenter_renderer_params.render_frequency.value());
    renderer_params.push_back(std::move(param));
  }
  if (presenter_renderer_params.shadow_technique.has_value()) {
    fuchsia::ui::gfx::RendererParam param;
    param.set_shadow_technique(presenter_renderer_params.shadow_technique.value());
    renderer_params.push_back(std::move(param));
  }

  async::Loop loop(&kAsyncLoopConfigAttachToCurrentThread);
  auto component_context = sys::ComponentContext::Create();

  // Ask the presenter to change renderer params.
  auto presenter = component_context->svc()->Connect<fuchsia::ui::policy::Presenter>();
  presenter.set_error_handler([&loop](zx_status_t status) {
    FXL_LOG(INFO) << "Lost connection to Presenter service.";
    loop.Quit();
  });
  presenter->HACK_SetRendererParams(clipping_enabled, std::move(renderer_params));

  // Done!
  async::PostTask(loop.dispatcher(), [&loop] { loop.Quit(); });
  loop.Run();
  return 0;
}
