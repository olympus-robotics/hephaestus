//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>
#include <thread>
#include <utility>

#include <fmt/base.h>
#include <fmt/core.h>
#include <hephaestus/cli/program_options.h>
#include <hephaestus/ipc/zenoh/ipc_graph.h>
#include <hephaestus/ipc/zenoh/program_options.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/utils/signal_handler.h>
#include <hephaestus/utils/stack_trace.h>

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>(heph::DEBUG));

  try {
    auto desc = heph::cli::ProgramDescription("IPC Graph monitoring example");
    heph::ipc::zenoh::appendProgramOption(desc);
    desc.defineOption<int>("duration", 'd', "Duration to listen for IPC events in seconds", 10);
    desc.defineFlag("live", 'l', "If set, the app will continue running until interrupted");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, _, __] = heph::ipc::zenoh::parseProgramOptions(args);
    const auto duration_sec = args.getOption<int>("duration");
    const bool live_mode = args.getOption<bool>("live");

    auto session = heph::ipc::zenoh::createSession(session_config);

    if (live_mode) {
      fmt::println("Starting IPC graph monitoring in live mode (press Ctrl+C to stop)...");
    } else {
      fmt::println("Starting IPC graph monitoring for {} seconds...", duration_sec);
    }

    // Set up IPC graph callbacks
    heph::ipc::zenoh::IpcGraphCallbacks callbacks;

    callbacks.topic_discovery_cb = [](const std::string& topic, const heph::serdes::TypeInfo& type_info) {
      fmt::println("Topic discovered: {}, Type: {}", topic, type_info.name);
    };

    callbacks.topic_removal_cb = [](const std::string& topic) { fmt::println("Topic removed: {}", topic); };

    callbacks.service_discovery_cb = [](const std::string& service_name,
                                        const heph::serdes::ServiceTypeInfo& service_type_info) {
      fmt::println("Service discovered: {}, Request Type: {}, Reply Type: {}", service_name,
                   service_type_info.request.name, service_type_info.reply.name);
    };

    callbacks.service_removal_cb = [](const std::string& service_name) {
      fmt::println("Service removed: {}", service_name);
    };

    callbacks.graph_update_cb = [](const heph::ipc::zenoh::EndpointInfo& info,
                                   const heph::ipc::zenoh::IpcGraphState& state) {
      fmt::println("Graph updated: {}", info.topic);
      state.printIpcGraphState();
    };

    // Create and start IPC graph
    heph::ipc::zenoh::IpcGraphConfig graph_config{ .session = session,
                                                   .track_topics_based_on_subscribers = true };

    heph::ipc::zenoh::IpcGraph ipc_graph(std::move(graph_config), std::move(callbacks));
    ipc_graph.start();

    // Register termination handler
    heph::utils::TerminationBlocker::registerInterruptCallback([&ipc_graph]() {
      fmt::println("\nStopping IPC graph monitoring...");
      ipc_graph.stop();
    });

    if (live_mode) {
      // In live mode, wait indefinitely for an interrupt
      heph::utils::TerminationBlocker::waitForInterrupt();
    } else {
      // Use the original timeout-based approach
      const auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(duration_sec);
      while (std::chrono::steady_clock::now() < end_time &&
             !heph::utils::TerminationBlocker::stopRequested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }

    // Stop IPC graph
    ipc_graph.stop();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return EXIT_FAILURE;
  }
}
