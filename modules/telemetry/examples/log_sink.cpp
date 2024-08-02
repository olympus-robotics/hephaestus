//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/absl_log_sink.h"
#include "hephaestus/telemetry/struclog.h"
#include "hephaestus/utils/stack_trace.h"

namespace ht = heph::telemetry;
// NOLINTBEGIN google-build-using-namespace
using namespace heph::telemetry::literals;
// NOLINTEND google-build-using-namespace

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, const char* /*argv*/[]) -> int {
  const heph::utils::StackTrace stack;

  const std::string a = "testing absl log sink";
  const int num = 123;
  const auto log_entry = ht::LogEntry{ ht::Level::Warn, a } | "num"_f(num);
  ht::registerLogSink(std::make_unique<ht::AbslLogSink>());
  ht::log(log_entry);
}
