//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"
#include "zenoh_program_options.h"

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  try {
    auto desc = heph::cli::ProgramDescription("String service server example");
    heph::ipc::zenoh::appendProgramOption(desc, getDefaultTopic(ExampleType::SERVICE_SERVER));
    const auto args = std::move(desc).parse(argc, argv);
    auto [session_config, topic_config] = heph::ipc::zenoh::parseProgramOptions(args);

    auto session = heph::ipc::zenoh::createSession(std::move(session_config));

    auto callback = [](const std::string& sample) {
      heph::log(heph::DEBUG, "received query", "query", sample);
      return (sample == "Marco") ? "Polo" : "What?";
    };

    const heph::ipc::zenoh::Service<std::string, std::string> server(session, topic_config, callback);

    heph::log(heph::DEBUG, "string server started, waiting for queries", "topic", topic_config.name);

    heph::utils::TerminationBlocker::waitForInterrupt();

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore =
        std::fputs(fmt::format("main terminated with an exception: {}\n", ex.what()).c_str(), stderr);
    return EXIT_FAILURE;
  }
}
