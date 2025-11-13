// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <utility>

#include <fmt/base.h>

#include "hephaestus/bag/writer.h"
#include "hephaestus/bag/zenoh_recorder.h"
#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;
  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("Record a bag from zenoh topics");
    heph::ipc::zenoh::appendProgramOption(desc);
    desc.defineOption<std::string>("output_bag", 'o', "output file where to write the bag");
    const auto args = std::move(desc).parse(argc, argv);
    auto [config, _, topic_filter] = heph::ipc::zenoh::parseProgramOptions(args);
    auto output_file = args.getOption<std::string>("output_bag");

    heph::bag::ZenohRecorderParams params{
      .session = heph::ipc::zenoh::createSession(config),
      .bag_writer = heph::bag::createMcapWriter({ .output_file = std::move(output_file) }),
      .topics_filter_params = topic_filter,
    };

    auto zeno_recorder = heph::bag::ZenohRecorder::create(std::move(params));
    zeno_recorder.start().get();

    heph::utils::TerminationBlocker::waitForInterrupt();

    zeno_recorder.stop().get();
  } catch (std::exception& e) {
    fmt::println("Failed with exception: {}", e.what());
    std::exit(1);
  }
}
