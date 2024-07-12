//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/sink.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <fmt/core.h>
#include <nlohmann/json.hpp>

namespace heph::telemetry {
// NOTE: we can use nhlosom::json to serialize the data, but for this example we are using fmt::format.
auto toJSON(const MetricEntry& log) -> std::string {
  return fmt::format(
      R"({{"component": "{}", "tag": "{}", "log_timestamp_ns": {}, "json_values": {}}})", log.component,
      log.tag,
      std::chrono::duration_cast<std::chrono::nanoseconds>(log.log_timestamp.time_since_epoch()).count(),
      log.json_values);
}

void fromJSON(std::string_view json, MetricEntry& log) {
  const auto j = nlohmann::json::parse(json);
  log.component = j.at("component").get<std::string>();
  log.tag = j.at("tag").get<std::string>();
  log.log_timestamp = ClockT::time_point{ std::chrono::duration_cast<ClockT::duration>(
      std::chrono::nanoseconds{ j.at("log_timestamp_ns").get<int64_t>() }) };
  log.json_values = j.at("json_values").get<std::string>();
}
}  // namespace heph::telemetry
