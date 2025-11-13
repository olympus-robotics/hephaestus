//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>

#include <fmt/base.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/examples/types/sample.h"
#include "hephaestus/examples/types_proto/sample.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

namespace {
[[nodiscard]] auto request(const heph::examples::types::SampleRequest& sample)
    -> heph::ipc::zenoh::action_server::TriggerStatus {
  heph::log(heph::DEBUG, "request received", "request", sample);
  if (sample.iterations_count == 0) {
    heph::log(heph::ERROR, "invalid request, iterations must be greater than 0");
    return heph::ipc::zenoh::action_server::TriggerStatus::REJECTED;
  }

  return heph::ipc::zenoh::action_server::TriggerStatus::SUCCESSFUL;
}

[[nodiscard]] auto
execute(const heph::examples::types::SampleRequest& request,
        heph::ipc::zenoh::Publisher<heph::examples::types::SampleReply>& status_update_publisher,
        std::atomic_bool& stop_requested) -> heph::examples::types::SampleReply {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 500 };
  heph::log(heph::DEBUG, "start execution", "iterations", request.iterations_count);
  std::size_t accumulated = request.initial_value;
  std::size_t counter = 0;
  for (; counter < request.iterations_count; ++counter) {
    if (stop_requested) {
      heph::log(heph::DEBUG, "stop requested, stopping execution");
      break;
    }

    accumulated += 1;
    const auto result = status_update_publisher.publish(
        heph::examples::types::SampleReply{ .value = accumulated, .counter = counter });
    heph::logIf(heph::ERROR, !result, "failed to publish status update");

    fmt::println("- Update {}: {} ", counter, accumulated);
    std::this_thread::sleep_for(WAIT_FOR);
  }

  return { .value = accumulated, .counter = counter };
}
}  // namespace

// We create a simple action server that accumulates a value for a given number of iterations.
// See VectorAccumulator class for more details.
// This example shows how to create an action server that receives a query and executes a task.
auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>(heph::DEBUG));

  try {
    auto desc = heph::cli::ProgramDescription("Action server example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::ACTION_SERVER));
    const auto args = std::move(desc).parse(argc, argv);
    auto [session_config, topic_config, _] = heph::ipc::zenoh::parseProgramOptions(args);

    auto stop_session_config = session_config;
    auto session = heph::ipc::zenoh::createSession(session_config);
    auto stop_session = heph::ipc::zenoh::createSession(stop_session_config);

    auto request_callback = [](const heph::examples::types::SampleRequest& sample) {
      return request(sample);
    };

    auto execute_callback = [](const heph::examples::types::SampleRequest& sample,
                               heph::ipc::zenoh::Publisher<heph::examples::types::SampleReply>& publisher,
                               std::atomic_bool& stop_requested) {
      return execute(sample, publisher, stop_requested);
    };

    const heph::ipc::zenoh::action_server::ActionServer<heph::examples::types::SampleRequest,
                                                        heph::examples::types::SampleReply,
                                                        heph::examples::types::SampleReply>
        action_server(session, topic_config, request_callback, execute_callback);

    heph::log(heph::DEBUG, "Action Server started, waiting for queries", "topic", topic_config.name);

    heph::utils::TerminationBlocker::registerInterruptCallback(
        [stop_session = std::ref(*stop_session), &topic_config] {
          std::ignore =
              heph::ipc::zenoh::action_server::requestActionServerToStopExecution(stop_session, topic_config);
        });

    heph::utils::TerminationBlocker::waitForInterrupt();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return EXIT_FAILURE;
  }
}
