//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <tuple>
#include <utility>

#include <absl/log/log.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/core.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  try {
    auto desc = heph::cli::ProgramDescription("Binary service client example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::SERVICE));
    const auto args = std::move(desc).parse(argc, argv);
    auto [session_config, topic_config] = heph::ipc::zenoh::parseProgramOptions(args);

    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    static constexpr auto K_TIMEOUT = std::chrono::seconds(10);
    const auto query =
        heph::examples::types::Pose{ .orientation = Eigen::Quaterniond{ 1., 0.3, 0.2, 0.1 },  // NOLINT
                                     .position = Eigen::Vector3d{ 3, 2, 1 } };                // NOLINT
    LOG(INFO) << fmt::format("Calling service on topic: {} with {}.", topic_config.name, query);
    const auto replies =
        heph::ipc::zenoh::callService<heph::examples::types::Pose, heph::examples::types::Pose>(
            *session, topic_config, query, K_TIMEOUT);
    if (!replies.empty()) {
      std::string reply_str;
      std::ranges::for_each(
          replies, [&reply_str](const auto& reply) { reply_str += fmt::format("-\t {}\n", reply.value); });
      fmt::println("Received: \n{}\n", reply_str);
    } else {
      LOG(ERROR) << "Error or no messages received after " << fmt::format("{}", K_TIMEOUT);
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
