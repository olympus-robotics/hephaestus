//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "hephaestus/telemetry/metrics/detail/struct_to_key_value_pairs.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"
#include "hephaestus/utils/timing/scoped_timer.h"
#include "hephaestus/utils/unique_function.h"

namespace heph::telemetry {

class MetricBuilder {
public:
  using SecondsT = std::chrono::duration<double>;

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

  template <typename ClockT = std::chrono::steady_clock>
  [[nodiscard]] auto measureScopeExecutionTime(std::string_view key) -> utils::timing::ScopedTimer {
    auto callback = [this, key](ClockT::duration duration) {
      metric_.values.emplace_back(fmt::format("{}.elapsed_s", key),
                                  std::chrono::duration_cast<SecondsT>(duration).count());
    };
    return utils::timing::ScopedTimer(std::move(callback), &ClockT::now);
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
