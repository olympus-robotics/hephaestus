//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/telemetry/metric_sink.h"

namespace heph::telemetry::sinks {
class TerminalMetricSink final : public heph::telemetry::IMetricSink {
public:
  void send(const heph::telemetry::Metric& metric) override;
};
}  // namespace heph::telemetry::sinks
