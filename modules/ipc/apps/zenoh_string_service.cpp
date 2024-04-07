//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <fmt/core.h>

#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Query");
    desc.defineOption<std::string>("value", 'v', "the value to pass the query", "");
    const auto args = std::move(desc).parse(argc, argv);
    const auto value = args.getOption<std::string>("value");

    auto [config, topic_config] = parseArgs(args);
    auto session = heph::ipc::zenoh::createSession(std::move(config));
    fmt::println("Simple service for std::string types for both Request and Reply. Don't use for services "
                 "with different types.");
    fmt::println("Opening session: {}", heph::ipc::zenoh::toString(session->zenoh_session.info_zid()));

    auto results = heph::ipc::zenoh::callService<std::string, std::string>(session, topic_config, value);

    std::ranges::for_each(
        results, [](const auto& res) { fmt::println(">> Received ('{}': '{}')", res.topic, res.value); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
