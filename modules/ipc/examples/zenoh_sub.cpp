//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <print>
#include <thread>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/cli/program_options.h"
#include "eolo/ipc/zenoh/subscriber.h"
#include "eolo/ipc/zenoh/utils.h"
#include "eolo/serdes/serdes.h"
#include "eolo/types/pose.h"
#include "eolo/types_protobuf/pose.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    static constexpr auto DEFAULT_KEY = "eolo/ipc/example/zenoh/put";

    auto desc = eolo::cli::ProgramDescription("Subscriber listening for data on specified key");
    desc.defineOption<std::string>("key", "Key expression", DEFAULT_KEY);

    const auto args = std::move(desc).parse(argc, argv);
    const auto key = args.getOption<std::string>("key");

    eolo::ipc::zenoh::Config config{ .topic = key, .cache_size = 100 };

    std::println("Opening session...");
    std::println("Declaring Subscriber on '{}'", key);
    const auto cb = [&key](const eolo::ipc::zenoh::MessageMetadata& metadata,
                           std::span<const std::byte> data) {
      eolo::types::Pose pose;
      eolo::serdes::deserialize(data, pose);
      std::println(">> Time: {}. Topic {}. From: {}. Counter: {}. Received {}",
                   std::chrono::system_clock::time_point{
                       std::chrono::duration_cast<std::chrono::system_clock::duration>(metadata.timestamp) },
                   key, metadata.sender_id, metadata.sequence_id, pose.position.transpose());
    };

    eolo::ipc::zenoh::Subscriber sub{ std::move(config), std::move(cb) };

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
