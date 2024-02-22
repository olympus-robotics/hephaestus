//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <fmt/core.h>

#include "eolo/ipc/zenoh/query.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/utils.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  try {
    auto desc = getProgramDescription("Query");
    desc.defineOption<std::string>("value", 'v', "the value to pass the query", "");
    const auto args = std::move(desc).parse(argc, argv);
    const auto value = args.getOption<std::string>("value");

    auto session = eolo::ipc::zenoh::createSession(parseArgs(args));
    fmt::println("Opening session: {}", eolo::ipc::zenoh::toString(session->zenoh_session.info_zid()));

    auto results = eolo::ipc::zenoh::query(session->zenoh_session, session->config.topic, value);

    std::ranges::for_each(
        results, [](const auto& res) { fmt::println(">> Received ('{}': '{}')", res.topic, res.value); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
