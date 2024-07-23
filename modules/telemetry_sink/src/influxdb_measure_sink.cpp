//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry_sink/influxdb_measure_sink.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <InfluxDB.h>
#include <InfluxDBFactory.h>
#include <Point.h>
#include <absl/log/log.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/telemetry/measure_sink.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::telemetry_sink {
namespace {

// We neeed to convert the JSON value to the original type.
// Supported types are: int64_t, double, string.
void addValueToPoint(influxdb::Point& point, const std::string& key, const nlohmann::json& value) {
  if (value.is_number_float()) {
    point.addField(key, value.get<double>());
  } else if (value.is_number_integer() || value.is_number_unsigned()) {
    point.addField(key, value.get<int64_t>());
  } else if (value.is_string()) {
    const auto value_str = value.get<std::string>();
    // According to JSON specification integer bigger than 32bits are converted to string, so we need to check
    // if the string is actually a number.
    if (const auto value_i = utils::string::stringTo<int64_t>(value_str); value_i.has_value()) {
      point.addField(key, value_i.value());
    } else {
      point.addField(key, value_str);
    }
  } else {
    LOG(ERROR) << fmt::format("Failed to add element {} with value {} to the influxdb point", key,
                              value.dump());
  }
}

}  // namespace

auto InfluxDBSink::create(InfluxDBSinkConfig config) -> std::unique_ptr<InfluxDBSink> {
  return std::unique_ptr<InfluxDBSink>(new InfluxDBSink(std::move(config)));
}

InfluxDBSink::InfluxDBSink(InfluxDBSinkConfig config) : config_(std::move(config)) {
  static constexpr auto URI_FORMAT = "http://{}@{}?db={}";
  const auto url = fmt::format(URI_FORMAT, config_.token, config_.url, config_.database);
  LOG(INFO) << fmt::format("Connecting to InfluxDB at {}", url);
  influxdb_ = influxdb::InfluxDBFactory::Get(url);
  if (config_.batch_size > 0) {
    influxdb_->batchOf(config_.batch_size);
  }
}

void InfluxDBSink::send(const telemetry::MeasureEntry& measure_entry) {
  auto point = influxdb::Point{ measure_entry.component }
                   .addTag("tag", measure_entry.tag)
                   .setTimestamp(measure_entry.measure_timestamp);

  const auto json_obj = nlohmann::json::parse(measure_entry.json_values);
  for (const auto& [key, value] : json_obj.items()) {
    addValueToPoint(point, key, value);
  }

  try {
    influxdb_->write(std::move(point));
  } catch (std::exception& e) {
    LOG(ERROR) << fmt::format("Failed to publish to InfluxDB: {}", e.what());
  }
}

}  // namespace heph::telemetry_sink
