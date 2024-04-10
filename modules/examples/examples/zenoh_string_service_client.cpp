//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/examples/types/pose.h"
#include "hephaestus/examples/types_protobuf/pose.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("String service client example", ExampleType::Service);
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    static constexpr auto K_TIMEOUT = std::chrono::seconds(10);
    const std::string query = "Marco";
    LOG(INFO) << fmt::format("Calling service on topic: {} with {}.", topic_config.name, query);
    const auto replies =
        heph::ipc::zenoh::callService<std::string, std::string>(*session, topic_config, query, K_TIMEOUT);
    if (!replies.empty()) {
      std::string reply_str;
      std::for_each(replies.begin(), replies.end(), [&reply_str](const auto& reply) {
        reply_str = fmt::format("{}\n-\t{}: {}", reply_str, reply.topic, reply.value);
      });
      LOG(INFO) << "Received:" << reply_str;
    } else {
      LOG(ERROR) << fmt::format("Error or no messages received after {}", K_TIMEOUT);
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
