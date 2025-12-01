//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"

namespace heph::telemetry {

/// @brief Helper class to compute a scope time duration and add it to a Metric.
/// The duration is added upon destruction of the scope.
/// Example usage:
/// ```cpp
/// {
///   Metric metric{
///     .component = "component",
///     .tag = "tag",
///     .timestamp = now,
///     .values = {};
///   };
///   {
///     auto _ = ScopedDurationRecorder{metric, "key"};
///     // code to measure
///   }
///   addKeyValue(metric, "key.value_key", 42);
/// }
/// ```
/// The above code create the following metrics entry:
/// * "key.value_key" : 42
/// * "key.elapsed_s" : <elapsed time in seconds>
class ScopedDurationRecorder {
public:
  using ClockT = std::chrono::steady_clock;
  using SecondsT = std::chrono::duration<double>;
  using NowFunctionPtr = std::chrono::steady_clock::time_point (*)();
  ScopedDurationRecorder(Metric& metric, std::string_view key, NowFunctionPtr now_fn = &ClockT::now)
    : metric_(&metric), now_fn_(now_fn), start_timestamp_(now_fn()), key_(fmt::format("{}.elapsed_s", key)) {
  }

  ~ScopedDurationRecorder() {
    const auto end_timestamp = now_fn_();
    const auto duration = end_timestamp - start_timestamp_;
    metric_->values.emplace_back(key_, std::chrono::duration_cast<SecondsT>(duration).count());
  }

  ScopedDurationRecorder(const ScopedDurationRecorder&) = delete;
  ScopedDurationRecorder(ScopedDurationRecorder&&) = delete;
  auto operator=(const ScopedDurationRecorder&) -> ScopedDurationRecorder& = delete;
  auto operator=(ScopedDurationRecorder&&) -> ScopedDurationRecorder& = delete;

private:
  Metric* metric_;
  NowFunctionPtr now_fn_{ nullptr };
  ClockT::time_point start_timestamp_;

  std::string key_;
};

/// @brief Helper class to publish a Metric upon destruction of the scope.
/// Example usage:
/// ```cpp
/// {
///   ScopedMetricPublisher publisher{
///     Metric{
///       .component = "component",
///       .tag = "tag",
///       .timestamp = now,
///       .values = {};
///     }
///   };
///   auto* metric = publisher.metric();
///   addKeyValue(*metric, "key.value", 42);
///   {
///     ScopedDurationRecorder recorder{*metric, "key"}
///   }
/// }
/// ```
/// The above code records the following metric entries:
/// * "key.value" : 42
/// * "key.elapsed_s" : <elapsed time in seconds>
class ScopedMetricPublisher {
public:
  explicit ScopedMetricPublisher(Metric&& metric) : metric_(std::move(metric)) {
  }

  ~ScopedMetricPublisher() {
    record(std::move(metric_));
  }

  [[nodiscard]] auto metric() -> Metric* {
    return &metric_;
  }

  ScopedMetricPublisher(const ScopedMetricPublisher&) = delete;
  ScopedMetricPublisher(ScopedMetricPublisher&&) = delete;
  auto operator=(const ScopedMetricPublisher&) -> ScopedMetricPublisher& = delete;
  auto operator=(ScopedMetricPublisher&&) -> ScopedMetricPublisher& = delete;

private:
  Metric metric_;
};

}  // namespace heph::telemetry
