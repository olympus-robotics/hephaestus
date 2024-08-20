//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/metric_sinks/terminal_sink.h"

#include <fmt/core.h>

#include "hephaestus/telemetry/metric_sink.h"

namespace heph::telemetry::metric_sinks {

void TerminalMetricSink::send(const Metric& metric) {
  fmt::println("{}", metric);
};
}  // namespace heph::telemetry::metric_sinks
