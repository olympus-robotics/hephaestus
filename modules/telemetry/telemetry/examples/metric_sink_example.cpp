//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/telemetry/metric_sink.h"
#include "hephaestus/telemetry/metric_sinks/terminal_sink.h"
#include "hephaestus/utils/stack_trace.h"
namespace ht = heph::telemetry;

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(int /*argc*/, const char* /*argv*/[]) -> int {
  const heph::utils::StackTrace stack;

  ht::registerMetricSink(std::make_unique<ht::metric_sinks::TerminalMetricSink>());

  auto mt = heph::random::createRNG();
  const auto entry = ht::Metric{ .component = heph::random::random<std::string>(mt),
                                 .tag = heph::random::random<std::string>(mt),
                                 .timestamp = heph::random::random<ht::ClockT::time_point>(mt),
                                 .values = { { heph::random::random<std::string>(mt),
                                               heph::random::random<int64_t>(mt) } } };
  ht::record(entry);
  std::cout << "Done\n" << std::flush;
  return 0;
}
