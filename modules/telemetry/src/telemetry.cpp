//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/telemetry.h"

#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {
auto Telemetry::instance() -> Telemetry& {
  static Telemetry telemetry;
  return telemetry;
}

void Telemetry::registerSink(ITelemetrySink* sink) {
  instance().sinks_.push_back(sink);
}

void Telemetry::log(const LogEntry& log_entry) {
  for (auto* sink : sinks_) {
    sink->send(log_entry);  // Maybe we should create a shared ptr for this, so we avoid making copies if we
                            // have multiple sinks.
  }
}
}  // namespace heph::telemetry
