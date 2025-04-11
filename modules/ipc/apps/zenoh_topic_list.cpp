//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string_view>
#include <tuple>
#include <utility>

#include <fmt/base.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/program_options.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

namespace {
void getListOfZenohEndpoints(const heph::ipc::zenoh::Session& session,
                             const heph::ipc::TopicFilter& topic_filter) {
  const auto publishers_info = heph::ipc::zenoh::getListOfEndpoints(session, topic_filter);
  std::ranges::for_each(publishers_info.begin(), publishers_info.end(), &heph::ipc::zenoh::printEndpointInfo);
}

void getLiveListOfZenohEndpoints(heph::ipc::zenoh::SessionPtr session, heph::ipc::TopicFilter topic_filter) {
  const heph::ipc::zenoh::EndpointDiscovery discover{ std::move(session), std::move(topic_filter),
                                                      &heph::ipc::zenoh::printEndpointInfo };

  heph::utils::TerminationBlocker::waitForInterrupt();
}
}  // namespace

auto main(int argc, const char* argv[]) -> int {
  const heph::utils::StackTrace stack_trace;
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc = heph::cli::ProgramDescription("List all the publishers of a topic.");
    heph::ipc::zenoh::appendProgramOption(desc);
    desc.defineFlag("live", 'l', "if set the app will keep running waiting for new publisher to advertise");
    const auto args = std::move(desc).parse(argc, argv);

    auto [session_config, _, topic_filter_params] = heph::ipc::zenoh::parseProgramOptions(args);
    topic_filter_params.exclude_prefix = "topic_info";
    auto topic_filter = heph::ipc::TopicFilter::create(topic_filter_params);

    fmt::println("Opening session...");
    auto session = heph::ipc::zenoh::createSession(session_config);

    if (!args.getOption<bool>("live")) {
      getListOfZenohEndpoints(*session, topic_filter);
    } else {
      getLiveListOfZenohEndpoints(session, std::move(topic_filter));
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
    return EXIT_FAILURE;
  }
}
