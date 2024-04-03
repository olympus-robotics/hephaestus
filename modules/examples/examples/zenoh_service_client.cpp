//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Periodic publisher example");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto type_info = heph::serdes::getSerializedTypeInfo<heph::examples::types::Pose>();

    const std::string topic = "test";
    static constexpr auto K_TIMEOUT = std::chrono::seconds(1);
    const auto query =
        heph::examples::types::Pose{ .orientation = Eigen::Quaterniond{ 1., 0.3, 0.2, 0.1 },  // NOLINT
                                     .position = Eigen::Vector3d{ 3, 2, 1 } };
    LOG(INFO) << fmt::format("Calling service on topic: {} with {}", topic,
                             heph::examples::types::toString(query));
    const auto reply =
        heph::ipc::zenoh::callService<heph::examples::types::Pose, heph::examples::types::Pose>(
            session, topic, query, K_TIMEOUT);
    if (reply) {
      LOG(INFO) << "Received: " << heph::examples::types::toString(reply.value());
    } else {
      LOG(ERROR) << "Timeout after " << fmt::format("{}", K_TIMEOUT);
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
