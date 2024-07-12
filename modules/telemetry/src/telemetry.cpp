//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/telemetry.h"

#include <mutex>

#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {
auto Telemetry::instance() -> Telemetry& {
  static Telemetry telemetry;
  return telemetry;
}

void Telemetry::registerSink(ITelemetrySink* sink) {
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  telemetry.sinks_.push_back(sink);
}

void Telemetry::metric(const MetricEntry& log_entry) {
  const std::lock_guard lock(sink_mutex_);
  for (auto* sink : sinks_) {
    sink->send(log_entry);  // Maybe we should create a shared ptr for this, so we avoid making copies if we
                            // have multiple sinks.
  }
}
}  // namespace heph::telemetry
