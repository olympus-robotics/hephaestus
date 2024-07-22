//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <type_traits>

#include <absl/base/thread_annotations.h>

#include "hephaestus/serdes/json.h"
#include "hephaestus/telemetry/sink.h"
#include "hephaestus/telemetry/struclog.h"

namespace heph::telemetry {

/// @brief Register a new telemetry sink.
/// For every metric logged, the sink will be called to send the data.
/// There is no limit on the number of sink supported.
void registerSink(std::unique_ptr<ITelemetrySink> sink);

/// @brief Generic metric logger.
void metric(const MetricEntry& log_entry);

/// @brief Logs a metric entry.
/// NOTE: the data needs to be serializable to JSON. For details on how to achieve this, see
/// `heph::serdes::serializeToJSON`.
template <typename DataT>
void metric(const std::string& component, const std::string& tag, const DataT& data,
            ClockT::time_point log_timestamp = ClockT::now()) {
  const MetricEntry log_entry{
    .component = component,
    .tag = tag,
    .log_timestamp = log_timestamp,
    .json_values = serdes::serializeToJSON(data),
  };

  metric(log_entry);
}

template <typename DataT>
  requires std::is_arithmetic_v<DataT> || std::is_same_v<DataT, std::string>
void metric(const std::string& component, const std::string& tag, const std::string& key, const DataT& value,
            ClockT::time_point log_timestamp = ClockT::now()) {
  const MetricEntry log_entry{
    .component = component,
    .tag = tag,
    .log_timestamp = log_timestamp,
    .json_values = fmt::format("{{\"{}\": {}}}", key, value),
  };

  metric(log_entry);
}

enum class Severity : std::uint8_t { Trace, Debug, Info, Warn, Error, Fatal };
void log(const Severity& s, const Log& l);

}  // namespace heph::telemetry
