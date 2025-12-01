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
template <typename ClockT = std::chrono::steady_clock>
class ScopedDurationRecorder {
public:
  using SecondsT = std::chrono::duration<double>;
  ScopedDurationRecorder(Metric& metric, std::string_view key)
    : metric_(&metric), start_timestamp_(ClockT::now()), key_(fmt::format("{}.elapsed_s", key)) {
  }

  ~ScopedDurationRecorder() {
    const auto end_timestamp = ClockT::now();
    const auto duration = end_timestamp - start_timestamp_;
    metric_->values.emplace_back(key_, std::chrono::duration_cast<SecondsT>(duration).count());
  }

  ScopedDurationRecorder(const ScopedDurationRecorder&) = delete;
  ScopedDurationRecorder(ScopedDurationRecorder&&) = delete;
  auto operator=(const ScopedDurationRecorder&) -> ScopedDurationRecorder& = delete;
  auto operator=(ScopedDurationRecorder&&) -> ScopedDurationRecorder& = delete;

private:
  Metric* metric_;
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
struct ScopedMetric : public Metric {
  using Metric::Metric;
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  ScopedMetric(Metric&& metric) : Metric(std::move(metric)) {
  }
  ~ScopedMetric() {
    record(std::move(*this));
  }

  ScopedMetric(const ScopedMetric&) = delete;
  ScopedMetric(ScopedMetric&&) = delete;
  auto operator=(const ScopedMetric&) -> ScopedMetric& = delete;
  auto operator=(ScopedMetric&&) -> ScopedMetric& = delete;
};

}  // namespace heph::telemetry
