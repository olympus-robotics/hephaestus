//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"
#include "hephaestus/utils/timing/scoped_timer.h"

namespace heph::telemetry {

/// @brief Helper class to build and record metrics.
/// The metric is recorded upon destruction of the builder.
/// Example usage:
/// ```cpp
/// {
///   MetricBuilder builder("component", "tag", ClockT::now());
///   builder.addValue("key", "value_key", 42);
///   {
///     auto timer = builder.measureScopeExecutionTime<>("key");
///     // code to measure
///   }
/// }
/// ```
/// The above code create the following metrics entry:
/// * "key1.value_key" : 42
/// * "key.elapsed_s" : <elapsed time in seconds>
class MetricBuilder {
public:
  using SecondsT = std::chrono::duration<double>;
  using ClockT = std::chrono::steady_clock;
  using NowFunctionPtr = std::chrono::steady_clock::time_point (*)();

  MetricBuilder(std::string component, std::string tag, std::chrono::system_clock::time_point timestamp)
    : metric_{
      .component = std::move(component),
      .tag = std::move(tag),
      .timestamp = timestamp,
      .values = {},
    } {
  }

  ~MetricBuilder() {
    record(std::move(metric_));
  }

  MetricBuilder(const MetricBuilder&) = default;
  MetricBuilder(MetricBuilder&&) = delete;
  auto operator=(const MetricBuilder&) -> MetricBuilder& = default;
  auto operator=(MetricBuilder&&) -> MetricBuilder& = delete;

  [[nodiscard]] auto measureScopeExecutionTime(std::string_view key, NowFunctionPtr now_fn = &ClockT::now)
      -> utils::timing::ScopedTimer {
    auto callback = [this, key](ClockT::duration duration) {
      metric_.values.emplace_back(fmt::format("{}.elapsed_s", key),
                                  std::chrono::duration_cast<SecondsT>(duration).count());
    };
    return utils::timing::ScopedTimer(std::move(callback), now_fn);
  }

  template <typename T>
  void addValue(std::string_view key, std::string_view value_key, T value) {
    metric_.values.emplace_back(
        Metric::KeyValueType{ fmt::format("{}.{}", key, value_key), std::move(value) });
  }

private:
  Metric metric_;
};

}  // namespace heph::telemetry
