//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <chrono>
#include <cstdlib>
#include <print>
#include <thread>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/cli/program_options.h"
#include "eolo/ipc/zenoh.h"
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

    zenohc::Config config;

    std::println("Opening session...");
    auto session = eolo::ipc::expect(open(std::move(config)));

    std::println("Declaring Subscriber on '{}'", key);
    const auto cb = [&key](const zenohc::Sample& sample) {
      eolo::types::Pose pose;
      // auto buffer = std::span<std::byte>()
      auto buffer = eolo::ipc::toByteSpan(sample.get_payload());
      eolo::serdes::deserialize(buffer, pose);
      std::println(">> Topic {}. Received {}", key, pose.position.transpose());
    };

    auto subs = eolo::ipc::expect(session.declare_subscriber(key, cb));
    std::println("Subscriber on '{}' declared", subs.get_keyexpr().as_string_view());

    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
