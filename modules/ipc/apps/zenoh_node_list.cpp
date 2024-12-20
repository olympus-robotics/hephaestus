//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <tuple>

#include <fmt/base.h>

#include "hephaestus/ipc/zenoh/scout.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/stack_trace.h"

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;

  const heph::utils::StackTrace stack_trace;

  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    fmt::println("Scouting..");

    auto nodes_info = heph::ipc::zenoh::getListOfNodes();
    std::ranges::for_each(nodes_info,
                          [](const auto& info) { fmt::println("{}", heph::ipc::zenoh::toString(info)); });

    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::ignore = std::fputs(ex.what(), stderr);
  }
}
