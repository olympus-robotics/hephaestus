//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <csignal>
#include <cstdlib>
#include <thread>

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/ipc/publisher.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/utils/exception.h"
#include "zenoh_program_options.h"

namespace {
volatile std::atomic_bool keep_running(true);  // NOLINT
}  // namespace

auto main(int argc, const char* argv[]) -> int {
  std::ignore = std::signal(SIGINT, [](int signal) {
    if (signal == SIGINT) {
      LOG(INFO) << "SIGINT received. Shutting down...";
      keep_running.store(false);
    }
  });
  try {
    auto desc = getProgramDescription("Periodic publisher example", ExampleType::Pubsub);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto type_info = heph::serdes::getSerializedTypeInfo<heph::examples::types::Pose>();
    heph::ipc::zenoh::Publisher publisher{ session, topic_config, type_info, [](const auto& status) {
                                            if (status.matching) {
                                              fmt::println("Subscriber match");
                                            } else {
                                              fmt::println("NO subscriber matching");
                                            }
                                          } };

    fmt::println("Declaring Publisher on '{}' with id: '{}'", topic_config.name, publisher.id());

    static constexpr auto LOOP_WAIT = std::chrono::seconds(1);
    double count = 0;
    while (keep_running.load()) {
      heph::examples::types::Pose pose;
      pose.position = Eigen::Vector3d{ 1, 2, count++ };
      pose.orientation =
          Eigen::Quaterniond{ 1., 0.1, 0.2, 0.3 };  // NOLINT(cppcoreguidelines-avoid-magic-numbers)

      fmt::println("Publishing Data ('{} : {})", topic_config.name, pose);
      auto res = heph::ipc::publish(publisher, pose);
      heph::throwExceptionIf<heph::InvalidOperationException>(!res, "failed to publish message");

      std::this_thread::sleep_for(LOOP_WAIT);
    }
    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
