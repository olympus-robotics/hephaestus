//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <InfluxDB.h>
#include <InfluxDBFactory.h>
#include <Point.h>
#include <absl/log/log.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {
namespace {
// TODO: maybe move this to some central place
template <typename T>
  requires std::is_same_v<T, int64_t> || std::is_same_v<T, double>
[[nodiscard]] auto stringTo(const std::string& str) -> std::optional<T> {
  const char* start = str.c_str();
  char* end{};
  errno = 0;
  T result{};
  if constexpr (std::is_same_v<T, int64_t>) {
    static constexpr int BASE = 10;
    result = std::strtoll(start, &end, BASE);
  } else {
    result = std::strtod(start, &end);
  }

  if (errno != ERANGE && *end == '\0') {
    return result;
  }

  return std::nullopt;
}

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
    if (const auto value_i = stringTo<int64_t>(value_str); value_i.has_value()) {
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

class InfluxDBSink final : public ITelemetrySink {
public:
  explicit InfluxDBSink(InfluxDBSinkConfig config);
  ~InfluxDBSink() override = default;

  void send(const MetricEntry& log_entry) override;

private:
  InfluxDBSinkConfig config_;
  std::unique_ptr<influxdb::InfluxDB> influxdb_;
};

InfluxDBSink::InfluxDBSink(InfluxDBSinkConfig config) : config_(std::move(config)) {
  static constexpr auto URI_FORMAT = "http://{}@{}?db={}";
  const auto url = fmt::format(URI_FORMAT, config_.token, config_.url, config_.database);
  LOG(INFO) << fmt::format("Connecting to InfluxDB at {}", url);
  influxdb_ = influxdb::InfluxDBFactory::Get(url);
  if (config_.batch_size > 0) {
    influxdb_->batchOf(config_.batch_size);
  }
}

void InfluxDBSink::send(const MetricEntry& log_entry) {
  auto point = influxdb::Point{ log_entry.component }
                   .addTag("tag", log_entry.tag)
                   .setTimestamp(log_entry.log_timestamp);

  const auto json_obj = nlohmann::json::parse(log_entry.json_values);
  for (const auto& [key, value] : json_obj.items()) {
    addValueToPoint(point, key, value);
  }

  try {
    influxdb_->write(std::move(point));
  } catch (std::exception& e) {
    LOG(ERROR) << fmt::format("Failed to publish to InfluxDB: {}", e.what());
  }
}

auto createInfluxDBSink(InfluxDBSinkConfig config) -> std::unique_ptr<ITelemetrySink> {
  return std::make_unique<InfluxDBSink>(std::move(config));
}

}  // namespace heph::telemetry
