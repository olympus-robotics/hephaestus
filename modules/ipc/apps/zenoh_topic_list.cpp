//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string_view>
#include <thread>
#include <tuple>
#include <utility>

#include <fmt/core.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

void getListOfPublisher(const heph::ipc::zenoh::Session& session, std::string_view topic) {
  const auto publishers_info = heph::ipc::zenoh::getListOfPublishers(session, topic);
  std::for_each(publishers_info.begin(), publishers_info.end(), &heph::ipc::zenoh::printPublisherInfo);
}

void getLiveListOfPublisher(heph::ipc::zenoh::SessionPtr session, heph::ipc::TopicConfig topic_config) {
  const heph::ipc::zenoh::PublisherDiscovery discover{ std::move(session), std::move(topic_config),
                                                       &heph::ipc::zenoh::printPublisherInfo };

  heph::utils::TerminationBlocker::waitForInterrupt();
}

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  try {
    auto desc = heph::cli::ProgramDescription("List all the publishers of a topic.");
    heph::ipc::zenoh::appendProgramOption(desc);
    desc.defineFlag("live", 'l', "if set the app will keep running waiting for new publisher to advertise");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = heph::ipc::zenoh::parseProgramOptions(args);

    fmt::println("Opening session...");
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    if (!args.getOption<bool>("live")) {
      getListOfPublisher(*session, topic_config.name);
    } else {
      getLiveListOfPublisher(std::move(session), std::move(topic_config));
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
