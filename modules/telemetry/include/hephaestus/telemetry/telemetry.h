//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <type_traits>

#include <absl/base/thread_annotations.h>

#include "hephaestus/serdes/json.h"
#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {

void registerSink(ITelemetrySink* sink);

void metric(const MetricEntry& log_entry);

template <serdes::JSONSerializable DataT>
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

}  // namespace heph::telemetry
