//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "hephaestus/serdes/json.h"
#include "hephaestus/telemetry/metric_sink.h"

namespace heph::telemetry {
namespace internal {
[[nodiscard]] auto jsonToValuesMap(std::string_view json)
    -> std::unordered_map<std::string, Metric::ValueType>;
}

/// @brief Register a new metric sink.
/// For every metric recorded, the sink will be called to consume the data.
/// There is no limit on the number of sinks supported.
void registerMetricSink(std::unique_ptr<IMetricSink> sink);

/// @brief Record a metric.
/// The metric is forwarded to all registered sinks.
/// Sinks process the metric in a dedicated thread, this means that this function is non-blocking and
/// deterministic.
void record(const Metric& metric);

/// @brief Record a user defined metric.
/// NOTE: the data needs to be serializable to JSON.
/// For details on how to achieve this, see `heph::serdes::serializeToJSON`.
/// @param component The component that is logging the metric_record, e.g. SLAM, Navigation, etc.
/// @param tag The tag of the metric_record used to identify who created it, e.g. "front_camera",
/// "motor1", etc.
/// @param data The data to record, needs to be json serializable.
/// @param timestamp The timestamp of the metric, if not provided, the current time is used.
template <typename DataT>
void record(const std::string& component, const std::string& tag, const DataT& data,
            ClockT::time_point timestamp = ClockT::now()) {
  const auto json = serdes::serializeToJSON(data);
  const Metric metric{
    .component = component,
    .tag = tag,
    .timestamp = timestamp,
    .values = internal::jsonToValuesMap(json),
  };

  record(metric);
}

}  // namespace heph::telemetry
