//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>
#include <string>

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

  const int num = 123;
  ht::log(ht::LogEntry{ ht::Level::WARN, "examples", "testing absl log sink" } | "num"_f(num));

  return 0;
}
