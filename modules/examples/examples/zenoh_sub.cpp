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
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  heph::utils::StackTrace stack_trace;

  try {
    auto desc = getProgramDescription("Periodic publisher example", ExampleType::Pubsub);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);

    LOG(INFO) << "Opening session...";
    LOG(INFO) << fmt::format("Declaring Subscriber on '{}'", topic_config.name);

    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto cb = [topic = topic_config.name](const heph::ipc::MessageMetadata& metadata,
                                          const std::shared_ptr<heph::examples::types::Pose>& pose) {
      fmt::println(">> Time: {}. Topic {}. From: {}. Counter: {}. Received {}",
                   std::chrono::system_clock::time_point{
                       std::chrono::duration_cast<std::chrono::system_clock::duration>(metadata.timestamp) },
                   metadata.topic, metadata.sender_id, metadata.sequence_id, *pose);
    };
    auto subscriber = heph::ipc::subscribe<heph::ipc::zenoh::Subscriber, heph::examples::types::Pose>(
        session, std::move(topic_config), std::move(cb));
    (void)subscriber;

    heph::utils::TerminationBlocker::waitForInterrupt();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
