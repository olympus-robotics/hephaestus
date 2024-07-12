//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/telemetry.h"

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {

class Telemetry {
public:
  static void registerSink(std::unique_ptr<ITelemetrySink> sink);

  static void metric(const MetricEntry& log_entry);

private:
  [[nodiscard]] static auto instance() -> Telemetry&;

private:
  std::mutex sink_mutex_;
  std::vector<std::unique_ptr<ITelemetrySink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
};

void registerSink(std::unique_ptr<ITelemetrySink> sink) {
  Telemetry::registerSink(std::move(sink));
}

void metric(const MetricEntry& log_entry) {
  Telemetry::metric(log_entry);
}

auto Telemetry::instance() -> Telemetry& {
  static Telemetry telemetry;
  return telemetry;
}

void Telemetry::registerSink(std::unique_ptr<ITelemetrySink> sink) {
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  telemetry.sinks_.push_back(std::move(sink));
}

void Telemetry::metric(const MetricEntry& log_entry) {
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  for (auto& sink : telemetry.sinks_) {
    sink->send(log_entry);  // Maybe we should create a shared ptr for this, so we avoid making copies if we
                            // have multiple sinks.
  }
}
}  // namespace heph::telemetry
