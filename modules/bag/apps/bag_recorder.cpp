//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <csignal>

#include "eolo/bag/writer.h"
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

  auto config = eolo::ipc::Config{};
  config.topic = args.getOption<std::string>("topic");
  auto session = eolo::ipc::zenoh::createSession(std::move(config));

  auto output_file = args.getOption<std::string>("output_bag");
  auto bag_writer = eolo::bag::createMcapWriter({ .output_file = std::move(output_file) });

  eolo::bag::ZenohRecorderParams params{
    .messages_queue_size = eolo::bag::ZenohRecorderParams::DEFAULT_MESSAGES_QUEUE_SIZE,
    .topic = session->config.topic,
    .session = std::move(session),
  };

  auto zeno_recorder = eolo::bag::ZenohRecorder::create(std::move(bag_writer), std::move(params));
  zeno_recorder.start().wait();

  stop_flag.wait(false);
  zeno_recorder.stop().wait();
}
