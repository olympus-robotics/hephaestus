//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <cstdlib>
#include <thread>

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/base/exception.h"
#include "eolo/examples/types/pose.h"
#include "eolo/examples/types_protobuf/pose.h"
#include "eolo/ipc/publisher.h"
#include "eolo/ipc/zenoh/publisher.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/serdes/serdes.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Periodic publisher example");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = eolo::ipc::zenoh::createSession(std::move(session_config));

    auto type_info = eolo::serdes::getSerializedTypeInfo<eolo::examples::types::Pose>();
    eolo::ipc::zenoh::Publisher publisher{ session, topic_config, type_info, [](const auto& status) {
                                            if (status.matching) {
                                              fmt::println("Subscriber match");
                                            } else {
                                              fmt::println("NO subscriber matching");
                                            }
                                          } };

    fmt::println("Declaring Publisher on '{}' with id: '{}'", topic_config.name, publisher.id());

    static constexpr auto LOOP_WAIT = std::chrono::seconds(1);
    double count = 0;
    while (true) {
      eolo::examples::types::Pose pose;
      pose.position = Eigen::Vector3d{ 1, 2, count++ };
      pose.orientation =
          Eigen::Quaterniond{ 1., 0.1, 0.2, 0.3 };  // NOLINT(cppcoreguidelines-avoid-magic-numbers)

      fmt::println("Publishing Data ('{} : {})", topic_config.name, pose);
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
