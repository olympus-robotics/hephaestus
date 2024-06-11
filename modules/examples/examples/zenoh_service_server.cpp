//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <csignal>
#include <cstdlib>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  heph::utils::StackTrace stack_trace;

  try {
    auto desc = getProgramDescription("Binary service server example", ExampleType::Service);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto callback = [](const heph::examples::types::Pose& sample) {
      LOG(INFO) << fmt::format("Received query: {}", sample);
      heph::examples::types::Pose sample_reply{
        .orientation = Eigen::Quaterniond{ 1., 0.1, 0.2, 0.3 },  // NOLINT
        .position = Eigen::Vector3d{ 1, 2, 3 },
      };
      return sample_reply;
    };

    heph::ipc::zenoh::Service<heph::examples::types::Pose, heph::examples::types::Pose> server(
        session, topic_config, callback);

    LOG(INFO) << fmt::format("Server started. Wating for queries on '{}' topic", topic_config.name);

    heph::utils::TerminationBlocker::waitForInterrupt();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
