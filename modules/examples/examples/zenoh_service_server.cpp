//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>
#include <thread>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Periodic publisher example");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto callback = [](const heph::examples::types::Pose& sample) {
      LOG(INFO) << "Received query: " << heph::examples::types::toString(sample);
      heph::examples::types::Pose sample_reply{
        .orientation = Eigen::Quaterniond{ 1., 0.1, 0.2, 0.3 },  // NOLINT
        .position = Eigen::Vector3d{ 1, 2, 3 },
      };
      return sample_reply;
    };

    heph::ipc::zenoh::Service<heph::examples::types::Pose, heph::examples::types::Pose> server(
        session, topic_config, callback);

    LOG(INFO) << fmt::format("Server started. Wating for queries on '{}' topic", topic_config.name);

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
