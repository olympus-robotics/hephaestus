//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>

#include <fmt/format.h>
#include <hephaestus/cli/program_options.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/utils/signal_handler.h>
#include <hephaestus/utils/stack_trace.h>
#include <hephaestus/websocket_bridge/bridge.h>
#include <hephaestus/websocket_bridge/bridge_config.h>

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("Hephaestus <===> WebSocket (Foxglove) WsBridge");
    desc.defineOption<std::filesystem::path>("config", 'c', "Path to the configuration YAML file");

    const auto args = std::move(desc).parse(argc, argv);
    heph::ws_bridge::WsBridgeConfig config;

    if (args.hasOption("config")) {
      auto config_file_path = args.getOption<std::filesystem::path>("config");
      if (!std::filesystem::exists(config_file_path)) {
        heph::log(heph::ERROR, "Config file not found", "path", config_file_path.string());
        return EXIT_FAILURE;
      }
      config = heph::ws_bridge::loadBridgeConfigFromYaml(config_file_path.string());
    } else {
      heph::log(heph::INFO, "Using default WebSocket Bridge configuration");
    }

    auto session = heph::ipc::zenoh::createSession(config.zenoh_config);
    auto bridge = std::make_unique<heph::ws_bridge::WsBridge>(session, config);

    bridge->start().get();

    heph::utils::TerminationBlocker::waitForInterrupt();

    bridge->stop().get();

  } catch (const std::exception& ex) {
    heph::log(heph::ERROR, "Failed with exception", "exception", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
