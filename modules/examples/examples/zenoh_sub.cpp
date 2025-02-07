//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <tuple>
#include <utility>

#include <fmt/base.h>
#include <fmt/chrono.h>  //NOLINT(misc-include-cleaner)

#include "hephaestus/cli/program_options.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_proto/pose.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/subscriber.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("Subscriber example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::PUBSUB));
    const auto args = std::move(desc).parse(argc, argv);
    auto [session_config, topic_config] = heph::ipc::zenoh::parseProgramOptions(args);

    heph::log(heph::DEBUG, "opening session", "subscriber_name", topic_config.name);

    session_config.id = "ALICE_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto cb = [topic = topic_config.name](const heph::ipc::zenoh::MessageMetadata& metadata,
                                          const std::shared_ptr<heph::examples::types::Pose>& pose) {
      fmt::println(">> Time: {}. Topic {}. From: {}. Counter: {}. Received {}",
                   std::chrono::system_clock::time_point{
                       std::chrono::duration_cast<std::chrono::system_clock::duration>(metadata.timestamp) },
                   metadata.topic, metadata.sender_id, metadata.sequence_id, *pose);
    };

    heph::ipc::zenoh::SubscriberConfig config;
    config.dedicated_callback_thread = true;
    auto subscriber = heph::ipc::zenoh::createSubscriber<heph::examples::types::Pose>(
        session, std::move(topic_config), std::move(cb), config);
    (void)subscriber;

    heph::utils::TerminationBlocker::waitForInterrupt();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
