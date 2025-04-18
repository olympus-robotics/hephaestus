//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>

#include <fmt/base.h>
#include <fmt/format.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_proto/pose.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc = heph::cli::ProgramDescription("Periodic publisher example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::PUBSUB));
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config, _] = heph::ipc::zenoh::parseProgramOptions(args);
    auto session = heph::ipc::zenoh::createSession(session_config);

    heph::ipc::zenoh::Publisher<heph::examples::types::Pose> publisher{ session, topic_config,
                                                                        [](const auto& status) {
                                                                          if (status.matching) {
                                                                            fmt::println("Subscriber match");
                                                                          } else {
                                                                            fmt::println(
                                                                                "NO subscriber matching");
                                                                          }
                                                                        } };

    fmt::println("[Session: '{}'] declaring RawPublisher on '{}'", publisher.sessionId(), topic_config.name);

    static constexpr auto LOOP_WAIT = std::chrono::seconds(1);
    double count = 0;
    while (!heph::utils::TerminationBlocker::stopRequested()) {
      heph::examples::types::Pose pose;
      pose.position = Eigen::Vector3d{ 1, 2, count++ };            // NOLINT
      pose.orientation = Eigen::Quaterniond{ 1., 0.1, 0.2, 0.3 };  // NOLINT

      fmt::println("Publishing Data ('{} : {})", topic_config.name, pose);
      auto res = publisher.publish(pose);
      heph::panicIf(!res, "failed to publish message");

      std::this_thread::sleep_for(LOOP_WAIT);
    }

    return EXIT_SUCCESS;

  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
