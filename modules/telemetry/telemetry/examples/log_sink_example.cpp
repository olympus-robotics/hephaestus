//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>
#include <string>

#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/utils/stack_trace.h"

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, const char* /*argv*/[]) -> int {
  using namespace std::literals::string_literals;

  const heph::utils::StackTrace stack;

  heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

  heph::log(heph::LogLevel::WARN, "testing absl log sink"s);

  const int num = 1234;
  heph::log(heph::LogLevel::INFO, "absl log sink with fields", "num", num, "quoted_string", "test");

  return 0;
}
