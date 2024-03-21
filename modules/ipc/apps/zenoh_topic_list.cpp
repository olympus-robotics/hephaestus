//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <csignal>

#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/session.h"

std::atomic_flag stop_flag = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
auto signalHandler(int /*unused*/) -> void {
  stop_flag.test_and_set();
  stop_flag.notify_all();
}

void getListOfPublisher(const heph::ipc::zenoh::Session& session, std::string_view topic) {
  const auto publishers_info = heph::ipc::zenoh::getListOfPublishers(session, topic);
  std::ranges::for_each(publishers_info,
                        [](const auto& info) { heph::ipc::zenoh::printPublisherInfo(info); });
}

void getLiveListOfPublisher(heph::ipc::zenoh::SessionPtr session, heph::ipc::TopicConfig topic_config) {
  auto callback = [](const auto& info) { heph::ipc::zenoh::printPublisherInfo(info); };

  heph::ipc::zenoh::PublisherDiscovery discover{ std::move(session), std::move(topic_config),
                                                 std::move(callback) };

  stop_flag.wait(false);
}

auto main(int argc, const char* argv[]) -> int {
  (void)signal(SIGINT, signalHandler);
  (void)signal(SIGTERM, signalHandler);

  try {
    auto desc = heph::ipc::zenoh::getProgramDescription("Periodic publisher example");
    desc.defineFlag("live", 'l', "if set the app will keep running waiting for new publisher to advertise");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = heph::ipc::zenoh::parseArgs(args);

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
