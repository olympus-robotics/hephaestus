//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/examples/types_proto/pose.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("String service client example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::SERVICE_SERVER));
    const auto args = std::move(desc).parse(argc, argv);
    auto [session_config, topic_config, _] = heph::ipc::zenoh::parseProgramOptions(args);

    auto session = heph::ipc::zenoh::createSession(session_config);

    static constexpr auto K_TIMEOUT = std::chrono::seconds(10);
    const std::string query = "Marco";
    heph::log(heph::DEBUG, "calling service", "topic", topic_config.name, "query", query);
    const auto replies =
        heph::ipc::zenoh::callService<std::string, std::string>(*session, topic_config, query, K_TIMEOUT);
    if (!replies.empty()) {
      std::string reply_str;
      std::ranges::for_each(replies, [&reply_str](const auto& reply) {
        reply_str = fmt::format("{}\n-\t{}: {}", reply_str, reply.topic, reply.value);
      });
      heph::log(heph::DEBUG, "received", "reply", reply_str);
    } else {
      heph::log(heph::ERROR, "error happened or no messages received", "timeout", K_TIMEOUT);
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
