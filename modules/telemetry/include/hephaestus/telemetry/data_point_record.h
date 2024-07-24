//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <string_view>

#include "hephaestus/serdes/json.h"
#include "hephaestus/telemetry/data_point_sink.h"

namespace heph::telemetry {
namespace internal {
[[nodiscard]] auto
jsonToValuesMap(std::string_view json) -> std::unordered_map<std::string, DataPoint::ValueType>;
}

/// @brief Register a new data point sink.
/// For every data point recorded, the sink will be called to consume the data.
/// There is no limit on the number of sinks supported.
void registerDataPointSink(std::unique_ptr<IDataPointSink> sink);

/// @brief Record a data point.
/// The data point is forwarded to all registered sinks.
/// Sinks process the data point in a dedicated thread, this means that this function is non-blocking and
/// deterministic.
void record(const DataPoint& data_point);

/// @brief Record a user defined data point.
/// NOTE: the data needs to be serializable to JSON.
/// For details on how to achieve this, see `heph::serdes::serializeToJSON`.
/// @param component The component that is logging the data_point_record, e.g. SLAM, Navigation, etc.
/// @param tag The tag of the data_point_record used to identify who created it, e.g. "front_camera",
/// "motor1", etc.
/// @param data The data to record, needs to be json serializable.
/// @param timestamp The timestamp of the data point, if not provided, the current time is used.
template <typename DataT>
void record(const std::string& component, const std::string& tag, const DataT& data,
            ClockT::time_point timestamp = ClockT::now()) {
  const auto json = serdes::serializeToJSON(data);
  const DataPoint data_point{
    .component = component,
    .tag = tag,
    .timestamp = timestamp,
    .values = internal::jsonToValuesMap(json),
  };

  record(data_point);
}

}  // namespace heph::telemetry
