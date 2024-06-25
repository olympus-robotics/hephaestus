// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>
#include <exception>
#include <future>
#include <string>
#include <utility>

#include <fmt/core.h>

#include "hephaestus/bag/writer.h"
#include "hephaestus/bag/zenoh_recorder.h"
#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/program_options.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

auto main(int argc, const char* argv[]) -> int {
  heph::utils::StackTrace stack_trace;

  try {
    auto desc = heph::cli::ProgramDescription("Record a bag from zenoh topics");
    heph::ipc::appendIPCProgramOption(desc);
    desc.defineOption<std::string>("output_bag", 'o', "output file where to write the bag");
    const auto args = std::move(desc).parse(argc, argv);
    auto [config, topic] = heph::ipc::parseIPCProgramOptions(args);
    auto output_file = args.getOption<std::string>("output_bag");

    heph::bag::ZenohRecorderParams params{
      .session = heph::ipc::zenoh::createSession(std::move(config)),
      .bag_writer = heph::bag::createMcapWriter({ .output_file = std::move(output_file) }),
      .topics_filter_params = heph::ipc::TopicFilterParams{ .include_topics_only = {},
                                                            .prefix = topic.name,
                                                            .exclude_topics = {} }
    };

    auto zeno_recorder = heph::bag::ZenohRecorder::create(std::move(params));
    zeno_recorder.start().wait();

    heph::utils::TerminationBlocker::waitForInterrupt();

    zeno_recorder.stop().get();

  } catch (std::exception& e) {
    fmt::println("Failed with exception: {}", e.what());
    std::exit(1);
  }
}
