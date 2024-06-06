//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <csignal>

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/program_options.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

void getListOfPublisher(const heph::ipc::zenoh::Session& session, std::string_view topic) {
  const auto publishers_info = heph::ipc::zenoh::getListOfPublishers(session, topic);
  std::ranges::for_each(publishers_info,
                        [](const auto& info) { heph::ipc::zenoh::printPublisherInfo(info); });
}

void getLiveListOfPublisher(heph::ipc::zenoh::SessionPtr session, heph::ipc::TopicConfig topic_config) {
  auto callback = [](const auto& info) { heph::ipc::zenoh::printPublisherInfo(info); };

  heph::ipc::zenoh::PublisherDiscovery discover{ std::move(session), std::move(topic_config),
                                                 std::move(callback) };

  heph::utils::InterruptHandler::wait();
}

auto main(int argc, const char* argv[]) -> int {
  heph::utils::StackTrace stack_trace;

  try {
    auto desc = heph::cli::ProgramDescription("List all the publishers of a topic.");
    heph::ipc::appendIPCProgramOption(desc);
    desc.defineFlag("live", 'l', "if set the app will keep running waiting for new publisher to advertise");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = heph::ipc::parseIPCProgramOptions(args);

    fmt::println("Opening session...");
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    if (!args.getOption<bool>("live")) {
      getListOfPublisher(*session, topic_config.name);
    } else {
      getLiveListOfPublisher(session, std::move(topic_config));
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
