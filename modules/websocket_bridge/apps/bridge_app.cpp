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
    // Define program options
    auto desc = heph::cli::ProgramDescription("Hephaestus <===> WebSocket (Foxglove) WsBridge");
    desc.defineOption<std::filesystem::path>("config", 'c', "Path to the configuration YAML file");

    // Parse command line arguments
    const auto args = std::move(desc).parse(argc, argv);
    heph::ws_bridge::WsBridgeConfig config;

    if (args.hasOption("config")) {
      auto config_file_path = args.getOption<std::filesystem::path>("config");
      // Load configuration from a YAML file
      config = heph::ws_bridge::loadBridgeConfigFromYaml(config_file_path.string());
    } else {
      // Create a default configuration
      config = heph::ws_bridge::WsBridgeConfig();
    }

    // Create a Zenoh session
    heph::ipc::zenoh::Config zenoh_config;
    zenoh_config.use_binary_name_as_session_id = false;
    zenoh_config.id = "websocket_bridge";
    zenoh_config.enable_shared_memory = false;
    zenoh_config.mode = heph::ipc::zenoh::Mode::PEER;
    zenoh_config.router = "";
    zenoh_config.cache_size = 0;
    zenoh_config.qos = false;
    zenoh_config.real_time = false;
    zenoh_config.protocol = heph::ipc::zenoh::Protocol::ANY;
    zenoh_config.multicast_scouting_enabled = true;
    zenoh_config.multicast_scouting_interface = "auto";
    auto session = heph::ipc::zenoh::createSession(zenoh_config);

    // Create the WsBridge instance
    auto bridge = std::make_unique<heph::ws_bridge::WsBridge>(session, config);

    bridge->start().get();

    // Spinn
    heph::utils::TerminationBlocker::waitForInterrupt();

    bridge->stop().get();
    bridge->wait();

  } catch (const std::exception& ex) {
    heph::log(heph::ERROR, "Failed with exception", "exception", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
