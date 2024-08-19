//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry_influxdb_sink/influxdb_metric_sink.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <InfluxDB.h>
#include <InfluxDBFactory.h>
#include <Point.h>
#include <absl/log/log.h>
#include <fmt/core.h>

#include "hephaestus/telemetry/metric_sink.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::telemetry_sink {
namespace {

[[nodiscard]] auto isNaN(const auto& value) -> bool {
  if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double> ||
                std::is_same_v<std::decay_t<decltype(value)>, int64_t>) {
    return std::isnan(value);
  } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>) {
    return utils::string::toLowerCase(value) == "nan";
  }

  return false;
}

[[nodiscard]] auto createInfluxdbPoint(const telemetry::Metric& entry) -> influxdb::Point {
  auto point = influxdb::Point{ entry.component }
                   .addTag("tag", entry.tag)
                   .addTag("id", std::to_string(entry.id))
                   .setTimestamp(entry.timestamp);
  for (const auto& [key, value] : entry.values) {
    std::visit(
        [&point, &key](auto&& arg) {
          if (isNaN(arg)) {
            return;
          }

          // When coming from JSON a value can be erronously first creted as int and then become double.
          // Influxdb forbid mixing types in the same field, so we need to convert every int to double.
          if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, int64_t>) {
            point.addField(key, static_cast<double>(arg));
            return;
          }

          point.addField(key, arg);
        },
        value);
  }

  return point;
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

InfluxDBSink::~InfluxDBSink() = default;

void InfluxDBSink::send(const telemetry::Metric& entry) {
  auto point = createInfluxdbPoint(entry);

  try {
    influxdb_->write(std::move(point));
  } catch (std::exception& e) {
    LOG(ERROR) << fmt::format("Failed to publish to InfluxDB: {}", e.what());
  }
}

}  // namespace heph::telemetry_sink
