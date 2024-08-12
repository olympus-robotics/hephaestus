//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <functional>
#include <tuple>
#include <utility>

#include <absl/log/log.h>
#include <fmt/core.h>
#include <magic_enum.hpp>

#include "hephaestus/examples/types/sample.h"
#include "hephaestus/examples/types_protobuf/sample.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  try {
    auto desc = getProgramDescription("Binary service client example", ExampleType::ACTION_SERVER);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    heph::utils::TerminationBlocker::registerInterruptCallback([session = std::ref(*session), &topic_config] {
      std::ignore = heph::ipc::zenoh::requestActionServerToStopExecution(session, topic_config);
    });

    auto status_update_cb = [](const heph::examples::types::SampleReply& sample) {
      LOG(INFO) << fmt::format("Received update: {}", sample);
    };

    static constexpr std::size_t START = 42;
    static constexpr std::size_t ITERATIONS = 10;
    const auto target = heph::examples::types::SampleRequest{
      .initial_value = START,
      .iterations_count = ITERATIONS,
    };
    auto result_future = heph::ipc::zenoh::callActionServer<heph::examples::types::SampleRequest,
                                                            heph::examples::types::SampleReply,
                                                            heph::examples::types::SampleReply>(
        session, topic_config, target, status_update_cb);

    LOG(INFO) << fmt::format("Call to Action Server (topic: '{}') started. Wating for result.",
                             topic_config.name);

    const auto result = result_future.get();

    LOG(INFO) << fmt::format("Received result: status {} | {}", magic_enum::enum_name(result.status),
                             result.value);

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return EXIT_FAILURE;
  }
}
