//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <type_traits>

#include "hephaestus/serdes/serdes.h"
#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {

class Telemetry {
public:
  static void registerSink(ITelemetrySink* sink);

  // Define the function input variable
  template <serdes::JSONSerializable DataT>
  static void metric(const std::string& component, const std::string& tag, const DataT& data,
                     ClockT::time_point log_timestamp = ClockT::now());

  template <typename DataT>
    requires std::is_arithmetic_v<DataT> || std::is_same_v<DataT, std::string>
  static void metric(const std::string& component, const std::string& tag, const std::string& key,
                     const DataT& value, ClockT::time_point log_timestamp = ClockT::now());

private:
  [[nodiscard]] static auto instance() -> Telemetry&;

  void metric(const MetricEntry& log_entry);

private:
  std::vector<ITelemetrySink*> sinks_;
};

template <serdes::JSONSerializable DataT>
void Telemetry::metric(const std::string& component, const std::string& tag, const DataT& data,
                       ClockT::time_point log_timestamp /*= ClockT::now()*/) {
  const MetricEntry log_entry{
    .component = component,
    .tag = tag,
    .log_timestamp = log_timestamp,
    .json_values = serdes::serializeToJSON(data),
  };

  instance().metric(log_entry);
}

template <typename DataT>
  requires std::is_arithmetic_v<DataT> || std::is_same_v<DataT, std::string>
void Telemetry::metric(const std::string& component, const std::string& tag, const std::string& key,
                       const DataT& value, ClockT::time_point log_timestamp /*= ClockT::now()*/) {
  const MetricEntry log_entry{
    .component = component,
    .tag = tag,
    .log_timestamp = log_timestamp,
    .json_values = fmt::format("{{\"{}\": {}}}", key, value),
  };

  instance().metric(log_entry);
}

}  // namespace heph::telemetry
