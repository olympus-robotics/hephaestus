//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/metrics/metric_record.h"

#include <exception>
#include <future>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/containers/blocking_queue.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/log_sink.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"
#include "hephaestus/utils/unique_function.h"

namespace heph::telemetry {

class MetricRecorder {
public:
  explicit MetricRecorder();
  ~MetricRecorder();
  MetricRecorder(const MetricRecorder&) = delete;
  MetricRecorder(MetricRecorder&&) = delete;
  auto operator=(const MetricRecorder&) -> MetricRecorder& = delete;
  auto operator=(MetricRecorder&&) -> MetricRecorder& = delete;

  static void registerSink(std::unique_ptr<IMetricSink> sink);

  static void record(UniqueFunction<Metric()>&& metric);

  static void flush();

private:
  [[nodiscard]] static auto instance() -> MetricRecorder&;
  void processEntry(const Metric& entry);

  /// @brief Empty the queue so that remaining messages get processed
  void emptyQueue();

private:
  absl::Mutex sink_mutex_;
  std::vector<std::unique_ptr<IMetricSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  static constexpr auto MAX_METRIC_QUEUE_SIZE = 100;
  containers::BlockingQueue<UniqueFunction<Metric()>> entries_;
  std::future<void> message_process_future_;
};

void registerMetricSink(std::unique_ptr<IMetricSink> sink) {
  MetricRecorder::registerSink(std::move(sink));
}

void record(UniqueFunction<Metric()>&& metric) {
  MetricRecorder::record(std::move(metric));
}

void record(Metric metric) {
  MetricRecorder::record([metric = std::move(metric)] { return metric; });
}

void flushMetrics() {
  MetricRecorder::flush();
}

MetricRecorder::MetricRecorder() : entries_{ MAX_METRIC_QUEUE_SIZE } {
  message_process_future_ = std::async(std::launch::async, [this]() {
    while (true) {
      auto message = entries_.waitAndPop();
      if (!message.has_value()) {
        break;
      }
      processEntry((*message)());
    }
    emptyQueue();
  });
}

MetricRecorder::~MetricRecorder() {
  try {
    entries_.stop();
    message_process_future_.get();
  } catch (const std::exception& ex) {
    log(ERROR, "emptying message consumer", "exception", ex.what());
  }
}

auto MetricRecorder::instance() -> MetricRecorder& {
  static MetricRecorder telemetry;
  return telemetry;
}

void MetricRecorder::registerSink(std::unique_ptr<IMetricSink> sink) {
  auto& telemetry = instance();
  const absl::MutexLock lock{ &telemetry.sink_mutex_ };
  telemetry.sinks_.push_back(std::move(sink));
}

void MetricRecorder::record(UniqueFunction<Metric()>&& metric) {
  auto& telemetry = instance();
  telemetry.entries_.forcePush(std::move(metric));
}

void MetricRecorder::flush() {
  auto& telemetry = instance();
  telemetry.emptyQueue();
}

void MetricRecorder::processEntry(const Metric& entry) {
  const absl::MutexLock lock{ &sink_mutex_ };
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

void MetricRecorder::emptyQueue() {
  while (!entries_.empty()) {
    auto message = entries_.tryPop();
    if (!message.has_value()) {
      return;
    }

    processEntry((*message)());
  }
}

}  // namespace heph::telemetry
