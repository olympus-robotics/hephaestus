//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <cstdlib>
#include <print>
#include <thread>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/base/exception.h"
#include "eolo/cli/program_options.h"
#include "eolo/ipc/zenoh/publisher.h"
#include "eolo/serdes/serdes.h"
#include "eolo/types/pose.h"
#include "eolo/types_protobuf/pose.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    static constexpr auto DEFAULT_KEY = "eolo/ipc/example/zenoh/put";

    auto desc = eolo::cli::ProgramDescription("Periodic publisher example");
    desc.defineOption<std::string>("key", "Key expression", DEFAULT_KEY);
    desc.defineOption<std::size_t>("cache", 'c', "Cache size", 0);

    const auto args = std::move(desc).parse(argc, argv);
    const auto key = args.getOption<std::string>("key");
    const auto cache_size = args.getOption<std::size_t>("cache");

    std::println("Declaring Publisher on '{}'", key);
    eolo::ipc::zenoh::Config config{ .topic = key, .cache_size = cache_size };
    eolo::ipc::zenoh::Publisher pub{ std::move(config) };

    static constexpr auto LOOP_WAIT = std::chrono::seconds(1);
    while (true) {
      eolo::types::Pose pose;
      pose.position = Eigen::Vector3d{ 1, 2, 3 };

      std::println("Publishing Data ('{} : {})", key, pose.position.transpose());
      auto buffer = eolo::serdes::serialize(pose);
      auto res = pub.publish(buffer);
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
