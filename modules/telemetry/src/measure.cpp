//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/measure.h"

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {

class Measure {
public:
  explicit Measure();
  static void registerSink(std::unique_ptr<IMeasureSink> sink);

  static void metric(const MeasureEntry& log_entry);

private:
  [[nodiscard]] static auto instance() -> Measure&;
  void processMeasureEntries(const MeasureEntry& entry);

private:
  std::mutex sink_mutex_;
  std::vector<std::unique_ptr<IMeasureSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  concurrency::MessageQueueConsumer<MeasureEntry> measure_entries_consumer_;
};

Measure::Measure()
  : measure_entries_consumer_([this](const MeasureEntry& entry) { processMeasureEntries(entry); }) {
}

void registerSink(std::unique_ptr<IMeasureSink> sink) {
  Measure::registerSink(std::move(sink));
}

void metric(const MeasureEntry& log_entry) {
  Measure::metric(log_entry);
}

auto Measure::instance() -> Measure& {
  static Measure telemetry;
  return telemetry;
}

void Measure::registerSink(std::unique_ptr<IMeasureSink> sink) {
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  telemetry.sinks_.push_back(std::move(sink));
}

void Measure::metric(const MeasureEntry& log_entry) {
  auto& telemetry = instance();
  telemetry.measure_entries_consumer_.queue().push(log_entry);
}

void Measure::processMeasureEntries(const MeasureEntry& entry) {
  const std::lock_guard lock(sink_mutex_);
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

}  // namespace heph::telemetry
