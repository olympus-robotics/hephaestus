//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/telemetry/metrics/metric_sink.h"

namespace heph::telemetry::metric_sinks {
class TerminalMetricSink final : public IMetricSink {
public:
  void send(const Metric& metric) override;
};
}  // namespace heph::telemetry::metric_sinks
