//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>
#include <string>

#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/telemetry/struclog.h"
#include "hephaestus/utils/stack_trace.h"

namespace ht = heph::telemetry;
// NOLINTBEGIN google-build-using-namespace
using namespace heph::telemetry::literals;
// NOLINTEND google-build-using-namespace

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, const char* /*argv*/[]) -> int {
  const heph::utils::StackTrace stack;

  ht::registerLogSink(std::make_unique<ht::AbslLogSink>());

  const std::string a = "testing absl log sink";
  const int num = 123;
  ht::log(ht::LogEntry{ ht::Level::WARN, a } | "num"_f(num));

  return 0;
}
