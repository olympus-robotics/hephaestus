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

#include "hephaestus/examples/types/accumulator.h"
#include "hephaestus/examples/types_protobuf/accumulator.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/publisher.h"
#include "hephaestus/ipc/zenoh/action_server.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/types/action_server_types.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

[[nodiscard]] auto
request(const heph::examples::types::Accumulator& sample) -> heph::ipc::zenoh::ActionServerRequestStatus {
  LOG(INFO) << fmt::format("Request received: value {} | iterations: {}", sample.value, sample.counter);
  if (sample.counter == 0) {
    LOG(ERROR) << "Invalid request, iterations must be greater than 0";
    return heph::ipc::zenoh::ActionServerRequestStatus::REJECTED_USER;
  }

  return heph::ipc::zenoh::ActionServerRequestStatus::ACCEPTED;
}

[[nodiscard]] auto execute(const heph::examples::types::Accumulator& sample,
                           heph::ipc::Publisher<heph::examples::types::Accumulator>& publisher,
                           std::atomic_bool& stop_requested) -> heph::examples::types::Accumulator {
  static constexpr auto WAIT_FOR = std::chrono::milliseconds{ 500 };
  LOG(INFO) << fmt::format("Start execution, iterations: {}", sample.counter);
  std::size_t accumulated = sample.value;
  for (std::size_t i = 0; i < sample.counter; ++i) {
    if (stop_requested) {
      LOG(INFO) << "Stop requested, stopping execution";
      break;
    }

    accumulated += 1;
    const auto result = publisher.publish(heph::examples::types::Accumulator{ accumulated, i });
    LOG_IF(ERROR, !result) << "Failed to publish status update";
    fmt::println("- Update {}: {} ", i, accumulated);
    std::this_thread::sleep_for(WAIT_FOR);
  }

  return { accumulated, sample.counter };
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
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto request_callback = [](const heph::examples::types::Accumulator& sample) { return request(sample); };

    auto execute_callback = [](const heph::examples::types::Accumulator& sample,
                               heph::ipc::Publisher<heph::examples::types::Accumulator>& publisher,
                               std::atomic_bool& stop_requested) {
      return execute(sample, publisher, stop_requested);
    };

    const heph::ipc::zenoh::ActionServer<heph::examples::types::Accumulator,
                                         heph::examples::types::Accumulator,
                                         heph::examples::types::Accumulator>
        action_server(session, topic_config, request_callback, execute_callback);

    LOG(INFO) << fmt::format("Action Server started. Wating for queries on '{}' topic", topic_config.name);

    heph::utils::TerminationBlocker::registerInterruptCallback([session = std::ref(*session), &topic_config] {
      std::ignore = heph::ipc::zenoh::requestActionServerToStopExecution(session, topic_config);
    });

    heph::utils::TerminationBlocker::waitForInterrupt();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return EXIT_FAILURE;
  }
}
