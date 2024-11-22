//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <memory>

#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/telemetry/struclog.h"
#include "hephaestus/utils/stack_trace.h"

namespace ht = heph::telemetry;

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, const char* /*argv*/[]) -> int {
  const heph::utils::StackTrace stack;

  ht::registerLogSink(std::make_unique<ht::AbslLogSink>());

  const int num = 1234;
  ht::log(ht::Level::WARN, "testing absl log sink");
  // NOLINTBEGIN(modernize-use-designated-initializers)
  ht::log(ht::Level::WARN, "testing absl log sink with field", ht::Field{ "num", num },
          ht::Field{ "quoted_string", "test" });
  // NOLINTEND(modernize-use-designated-initializers)
  return 0;
}
