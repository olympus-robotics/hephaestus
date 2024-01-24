//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <cstdlib>
#include <print>
#include <thread>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/base/exception.h"
#include "eolo/ipc/publisher.h"
#include "eolo/ipc/zenoh/publisher.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/serdes/serdes.h"
#include "eolo/types/pose.h"
#include "eolo/types_protobuf/pose.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Periodic publisher example");
    const auto args = std::move(desc).parse(argc, argv);

    auto config = parseArgs(args);
    auto session = eolo::ipc::zenoh::createSession(config);
    eolo::ipc::zenoh::Publisher publisher{ session, std::move(config) };

    std::println("Declaring Publisher on '{}' with id: '{}'", config.topic, publisher.id());

    static constexpr auto LOOP_WAIT = std::chrono::seconds(1);
    while (true) {
      eolo::types::Pose pose;
      pose.position = Eigen::Vector3d{ 1, 2, 3 };

      std::println("Publishing Data ('{} : {})", config.topic, pose.position.transpose());
      auto res = eolo::ipc::publish(publisher, pose);
      eolo::throwExceptionIf<eolo::InvalidOperationException>(!res, "failed to publish message");

      std::this_thread::sleep_for(LOOP_WAIT);
    }
    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(std::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
