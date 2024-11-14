//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <chrono>
#include <memory>
#include <thread>

#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/telemetry/struclog.h"
#include "hephaestus/utils/stack_trace.h"

namespace ht = heph::telemetry;
// NOLINTNEXTLINE google-build-using-namespace
using namespace heph::telemetry::literals;

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, const char* /*argv*/[]) -> int {
  const heph::utils::StackTrace stack;

  ht::registerLogSink(std::make_unique<ht::AbslLogSink>());

  ht::log(ht::LogEntry{ ht::Level::WARN, "examples", "testing absl log sink a" } | "num"_f(1234));
  ht::log({ ht::Level::WARN, "examples", "testing absl log sink" });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  return 0;
}
