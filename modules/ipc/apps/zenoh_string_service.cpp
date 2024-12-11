//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <tuple>
#include <utility>

#include <fmt/base.h>
#include <fmt/format.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/stack_trace.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc =
        heph::cli::ProgramDescription("Simple service for std::string types for both Request and Reply. "
                                      "Don't use for services with different types.");
    heph::ipc::zenoh::appendProgramOption(desc);
    desc.defineOption<std::string>("value", 'v', "the value to pass the query", "");
    const auto args = std::move(desc).parse(argc, argv);
    const auto value = args.getOption<std::string>("value");

    auto [config, topic_config] = heph::ipc::zenoh::parseProgramOptions(args);
    auto session = heph::ipc::zenoh::createSession(std::move(config));
    heph::log(heph::INFO, "opening session", "id",
              heph::ipc::zenoh::toString(session->zenoh_session.get_zid()));

    static constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds{ 1000 };
    auto results = heph::ipc::zenoh::callService<std::string, std::string>(*session, topic_config, value,
                                                                           DEFAULT_TIMEOUT);

    std::ranges::for_each(
        results, [](const auto& res) { fmt::println(">> Received ('{}': '{}')", res.topic, res.value); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
