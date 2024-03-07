// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <csignal>

#include <fmt/core.h>

#include "hephaestus/bag/zenoh_recorder.h"
#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/common.h"

std::atomic_flag stop_flag = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
auto signalHandler(int /*unused*/) -> void {
  stop_flag.test_and_set();
  stop_flag.notify_all();
}

auto main(int argc, const char* argv[]) -> int {
  (void)signal(SIGINT, signalHandler);
  (void)signal(SIGTERM, signalHandler);

  try {
    auto desc = heph::cli::ProgramDescription("Record a bag from zenoh topics");
    desc.defineOption<std::string>("topic", 't', "Topics to record", "**")
        .defineOption<std::string>("output_bag", 'o', "output file where to write the bag");
    const auto args = std::move(desc).parse(argc, argv);
    auto topic = args.getOption<std::string>("topic");
    auto output_file = args.getOption<std::string>("output_bag");

    heph::bag::ZenohRecorderParams params{
      .session = heph::ipc::zenoh::createSession({}),
      .bag_writer = heph::bag::createMcapWriter({ .output_file = std::move(output_file) }),
      .topics_filter_params =
          heph::bag::TopicFilterParams{ .include_topics_only = {}, .prefix = topic, .exclude_topics = {} }
    };

    auto zeno_recorder = heph::bag::ZenohRecorder::create(std::move(params));
    zeno_recorder.start().wait();

    stop_flag.wait(false);
    zeno_recorder.stop().wait();

  } catch (std::exception& e) {
    fmt::println("Failed with exception: {}", e.what());
    std::exit(1);
  }
}
