//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <thread>
#include <tuple>
#include <utility>

#include <absl/log/log.h>
#include <fmt/core.h>

#include "hephaestus/examples/types/sample.h"
#include "hephaestus/examples/types_protobuf/sample.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/publisher.h"
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

[[nodiscard]] auto request(const heph::examples::types::SampleRequest& sample)
    -> heph::ipc::zenoh::action_server::TriggerStatus {
  LOG(INFO) << fmt::format("Request received: {}", sample);
  if (sample.iterations_count == 0) {
    LOG(ERROR) << "Invalid request, iterations must be greater than 0";
    return heph::ipc::zenoh::action_server::TriggerStatus::REJECTED;
  }

  return heph::ipc::zenoh::action_server::TriggerStatus::SUCCESSFUL;
}

[[nodiscard]] auto execute(const heph::examples::types::SampleRequest& request,
                           heph::ipc::Publisher<heph::examples::types::SampleReply>& status_update_publisher,
                           std::atomic_bool& stop_requested) -> heph::examples::types::SampleReply {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 500 };
  LOG(INFO) << fmt::format("Start execution, iterations: {}", request.iterations_count);
  std::size_t accumulated = request.initial_value;
  std::size_t counter = 0;
  for (; counter < request.iterations_count; ++counter) {
    if (stop_requested) {
      LOG(INFO) << "Stop requested, stopping execution";
      break;
    }

    accumulated += 1;
    const auto result =
        status_update_publisher.publish(heph::examples::types::SampleReply{ accumulated, counter });
    LOG_IF(ERROR, !result) << "Failed to publish status update";
    fmt::println("- Update {}: {} ", counter, accumulated);
    std::this_thread::sleep_for(WAIT_FOR);
  }

  return { accumulated, counter };
}

// We create a simple action server that accumulates a value for a given number of iterations.
// See VectorAccumulator class for more details.
// This example shows how to create an action server that receives a query and executes a task.
auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  try {
    auto desc = getProgramDescription("ActionServer example", ExampleType::ACTION_SERVER);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto stop_session_config = session_config;
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));
    auto stop_session = heph::ipc::zenoh::createSession(std::move(stop_session_config));

    auto request_callback = [](const heph::examples::types::SampleRequest& sample) {
      return request(sample);
    };

    auto execute_callback = [](const heph::examples::types::SampleRequest& sample,
                               heph::ipc::Publisher<heph::examples::types::SampleReply>& publisher,
                               std::atomic_bool& stop_requested) {
      return execute(sample, publisher, stop_requested);
    };

    const heph::ipc::zenoh::action_server::ActionServer<heph::examples::types::SampleRequest,
                                                        heph::examples::types::SampleReply,
                                                        heph::examples::types::SampleReply>
        action_server(session, topic_config, request_callback, execute_callback);

    LOG(INFO) << fmt::format("Action Server started. Wating for queries on '{}' topic", topic_config.name);

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
