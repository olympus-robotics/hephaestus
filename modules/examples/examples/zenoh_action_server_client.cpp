//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>

#include <fmt/base.h>
#include <magic_enum.hpp>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/examples/types/sample.h"
#include "hephaestus/examples/types_proto/sample.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/action_server/action_server.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("Action server client example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::ACTION_SERVER));
    const auto args = std::move(desc).parse(argc, argv);
    auto [session_config, topic_config] = heph::ipc::zenoh::parseProgramOptions(args);

    auto session = heph::ipc::zenoh::createSession(session_config);

    heph::utils::TerminationBlocker::registerInterruptCallback([session = std::ref(*session), &topic_config] {
      std::ignore =
          heph::ipc::zenoh::action_server::requestActionServerToStopExecution(session, topic_config);
    });

    auto status_update_cb = [](const heph::examples::types::SampleReply& sample) {
      heph::log(heph::DEBUG, "received update", "reply", sample);
    };

    static constexpr std::size_t START = 42;
    static constexpr std::size_t ITERATIONS = 10;
    const auto target = heph::examples::types::SampleRequest{
      .initial_value = START,
      .iterations_count = ITERATIONS,
    };
    static constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds{ 1000 };
    auto result_future =
        heph::ipc::zenoh::action_server::callActionServer<heph::examples::types::SampleRequest,
                                                          heph::examples::types::SampleReply,
                                                          heph::examples::types::SampleReply>(
            session, topic_config, target, status_update_cb, DEFAULT_TIMEOUT);

    heph::log(heph::DEBUG, "call to Action Server started, waiting for result", "topic", topic_config.name);

    const auto result = result_future.get();

    heph::log(heph::DEBUG, "received result", "status", magic_enum::enum_name(result.status), "value",
              result.value);

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    fmt::println(stderr, "main terminated with an exception: {}\n", ex.what());
    return EXIT_FAILURE;
  }
}
