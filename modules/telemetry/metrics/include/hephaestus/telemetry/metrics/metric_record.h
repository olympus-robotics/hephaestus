//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "hephaestus/telemetry/metrics/detail/struct_to_key_value_pairs.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"
#include "hephaestus/utils/unique_function.h"

namespace heph::telemetry {
/// @brief Register a new metric sink.
/// For every metric recorded, the sink will be called to consume the data.
/// There is no limit on the number of sinks supported.
void registerMetricSink(std::unique_ptr<IMetricSink> sink);

/// @brief Record a metric.
/// The metric is forwarded to all registered sinks.
/// Sinks process the metric in a dedicated thread, this means that this function is non-blocking and
/// deterministic.
void record(Metric metric);
void record(heph::UniqueFunction<Metric()>&& metric);

/// @brief Record a user defined metric.
/// NOTE: the data needs to be serializable to JSON.
/// For details on how to achieve this, see `heph::serdes::serializeToJSON`.
/// @param component The component that is logging the metric_record, e.g. SLAM, Navigation, etc.
/// @param tag The tag of the metric_record used to identify who created it, e.g. "front_camera",
/// "motor1", etc.
/// @param data The data to record, needs to be json serializable.
/// @param timestamp The timestamp of the metric, if not provided, the current time is used.
template <typename DataT>
void record(std::string component, std::string tag, DataT&& data,
            ClockT::time_point timestamp = ClockT::now()) {
  record(
      [component = std::move(component), tag = std::move(tag), data = std::forward<DataT>(data), timestamp] {
        return Metric{
          .component = std::move(component),
          .tag = std::move(tag),
          .timestamp = timestamp,
          .values = detail::structToKeyValuePairs(data),
        };
      });
}

void flushMetrics();

}  // namespace heph::telemetry
