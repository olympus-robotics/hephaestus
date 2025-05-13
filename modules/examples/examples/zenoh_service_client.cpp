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

#include <fmt/base.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_proto/pose.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/service_client.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("Binary service client example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::SERVICE_SERVER));
    const auto args = std::move(desc).parse(argc, argv);
    auto [session_config, topic_config, _] = heph::ipc::zenoh::parseProgramOptions(args);

    auto session = heph::ipc::zenoh::createSession(session_config);

    static constexpr auto K_TIMEOUT = std::chrono::seconds(10);
    const auto query =
        heph::examples::types::Pose{ .orientation = Eigen::Quaterniond{ 1., 0.3, 0.2, 0.1 },  // NOLINT
                                     .position = Eigen::Vector3d{ 3, 2, 1 } };                // NOLINT
    heph::log(heph::DEBUG, "calling service", "topic", topic_config.name, "query", query);
    auto service_client =
        heph::ipc::zenoh::ServiceClient<heph::examples::types::Pose, heph::examples::types::Pose>(
            session, topic_config, K_TIMEOUT);

    const auto replies = service_client.call(query);
    if (!replies.empty()) {
      std::string reply_str;
      std::ranges::for_each(
          replies, [&reply_str](const auto& reply) { reply_str += fmt::format("-\t {}\n", reply.value); });
      fmt::println("Received: \n{}\n", reply_str);
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
