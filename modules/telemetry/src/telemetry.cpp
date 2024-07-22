//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/telemetry.h"

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/log/log.h>

#include "hephaestus/telemetry/sink.h"
#include "hephaestus/telemetry/struclog.h"

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

void log(const Severity& s, const Log& l) {
  switch (s) {
    case Severity::Trace:
      ABSL_VLOG(2) << l;
      break;
    case Severity::Debug:
      ABSL_VLOG(1) << l;
      break;
    case Severity::Info:
      ABSL_LOG(INFO) << l;
      break;
    case Severity::Warn:
      ABSL_LOG(WARNING) << l;
      break;
    case Severity::Error:
      ABSL_LOG(ERROR) << l;
      break;
    case Severity::Fatal:
      ABSL_LOG(FATAL) << l;
      break;
  }
}
}  // namespace heph::telemetry
