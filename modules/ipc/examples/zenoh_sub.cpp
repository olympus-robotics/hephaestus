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

    zenohc::Config config;

    std::println("Opening session...");
    auto session = eolo::ipc::zenoh::expect(open(std::move(config)));

    std::println("Declaring Subscriber on '{}'", key);
    const auto cb = [&key](const zenohc::Sample& sample) {
      int counter = 0;
      if (sample.get_attachment().check()) {
        auto counter_str = std::string{ sample.get_attachment().get("msg_counter").as_string_view() };
        counter = std::stoi(counter_str);
      }
      eolo::types::Pose pose;
      auto buffer = eolo::ipc::zenoh::toByteSpan(sample.get_payload());
      eolo::serdes::deserialize(buffer, pose);
      std::println(">> Time: {}. Topic {}. From: {}. Counter: {}. Received {}",
                   eolo::ipc::zenoh::toChrono(sample.get_timestamp()), key,
                   sample.get_keyexpr().as_string_view(), counter, pose.position.transpose());
    };

    auto subs = eolo::ipc::zenoh::expect(session.declare_subscriber(key, cb));
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
