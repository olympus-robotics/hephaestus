//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <thread>

#include <absl/log/log.h>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/ipc/subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
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

    LOG(INFO) << "Opening session...";
    LOG(INFO) << fmt::format("Declaring Subscriber on '{}'", topic_config.name);

    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto cb = [topic = topic_config.name](const heph::ipc::MessageMetadata& metadata,
                                          const std::shared_ptr<heph::examples::types::Pose>& pose) {
      LOG(INFO) << fmt::format(
          ">> Time: {}. Topic {}. From: {}. Counter: {}. Received {}",
          std::chrono::system_clock::time_point{
              std::chrono::duration_cast<std::chrono::system_clock::duration>(metadata.timestamp) },
          metadata.topic, metadata.sender_id, metadata.sequence_id, *pose);
    };
    auto subscriber = heph::ipc::subscribe<heph::ipc::zenoh::Subscriber, heph::examples::types::Pose>(
        session, std::move(topic_config), std::move(cb));
    (void)subscriber;

    while (keep_running.load()) {
      std::this_thread::sleep_for(std::chrono::seconds{ 1 });
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
