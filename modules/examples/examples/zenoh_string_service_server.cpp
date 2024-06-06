//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <csignal>
#include <cstdlib>
#include <thread>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

std::atomic_flag stop_flag = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
auto signalHandler(int /*unused*/) -> void {
  stop_flag.test_and_set();
  stop_flag.notify_all();
}

auto main(int argc, const char* argv[]) -> int {
  (void)signal(SIGINT, signalHandler);
  (void)signal(SIGTERM, signalHandler);

  heph::utils::StackTrace stack_trace;

  try {
    auto desc = getProgramDescription("String service server example", ExampleType::Service);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto callback = [](const std::string& sample) {
      LOG(INFO) << "Received query: " << sample;
      std::string reply = (sample == "Marco") ? "Polo" : "What?";
      return reply;
    };

    heph::ipc::zenoh::Service<std::string, std::string> server(session, topic_config, callback);

    LOG(INFO) << fmt::format("String server started. Wating for queries on '{}' topic", topic_config.name);

    stop_flag.wait(false);

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
