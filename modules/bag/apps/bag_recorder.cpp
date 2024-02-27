// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <csignal>

#include "eolo/bag/zenoh_recorder.h"
#include "eolo/cli/program_options.h"
#include "eolo/ipc/common.h"

std::atomic_flag stop_flag = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
auto signalHandler(int /*unused*/) -> void {
  stop_flag.test_and_set();
  stop_flag.notify_all();
}

auto main(int argc, const char* argv[]) -> int {
  (void)signal(SIGINT, signalHandler);
  (void)signal(SIGTERM, signalHandler);

  auto desc = eolo::cli::ProgramDescription("Record a bag from zenoh topics");
  desc.defineOption<std::string>("topic", 't', "Topics to record", "**")
      .defineOption<std::string>("output_bag", 'o', "output file where to write the bag");
  const auto args = std::move(desc).parse(argc, argv);
  auto topic = args.getOption<std::string>("topic");
  auto output_file = args.getOption<std::string>("output_bag");

  auto config = eolo::ipc::Config{};
  config.topic = topic;

  eolo::bag::ZenohRecorderParams params{
    .session = eolo::ipc::zenoh::createSession(std::move(config)),
    .bag_writer = eolo::bag::createMcapWriter({ .output_file = std::move(output_file) }),
    .topics_filter_params =
        eolo::bag::TopicFilterParams{ .include_topics_only = {}, .prefix = topic, .exclude_topics = {} }
  };

  auto zeno_recorder = eolo::bag::ZenohRecorder::create(std::move(params));
  zeno_recorder.start().wait();

  stop_flag.wait(false);
  zeno_recorder.stop().wait();
}
