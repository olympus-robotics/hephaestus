//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/json.h"
#include "hephaestus/telemetry/measure_sink.h"

namespace heph::telemetry {

/// @brief Register a new telemetry sink.
/// For every metric logged, the sink will be called to send the data.
/// There is no limit on the number of sink supported.
void registerSink(std::unique_ptr<IMeasureSink> sink);

/// @brief Generic metric logger.
void measure(const MeasureEntry& measure_entry);

/// @brief Logs a metric entry.
/// NOTE: the data needs to be serializable to JSON. For details on how to achieve this, see
/// `heph::serdes::serializeToJSON`.
template <typename DataT>
void measure(const std::string& component, const std::string& tag, const DataT& data,
             ClockT::time_point measure_timestamp = ClockT::now()) {
  const MeasureEntry measure_entry{
    .component = component,
    .tag = tag,
    .measure_timestamp = measure_timestamp,
    .json_values = serdes::serializeToJSON(data),
  };

  measure(measure_entry);
}

}  // namespace heph::telemetry
