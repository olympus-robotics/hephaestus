//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#include <InfluxDB.h>
#include <InfluxDBFactory.h>
#include <Point.h>
#include <absl/log/log.h>
#include <cpr/cpr.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/telemetry/sink.h"
#include "proto_conversion.h"  // NOLINT(misc-include-cleaner)

namespace heph::telemetry {
namespace {
template <typename T>  // TODO: add requires T primitive type
  requires std::is_integral_v<T>
[[nodiscard]] auto stringTo(const std::string& str) -> std::optional<T> {
  T result{};
  const auto* begin = str.data();
  const auto* end = begin + str.size();  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

  auto [ptr, ec] = std::from_chars(begin, end, result);

  if (ec == std::errc() && ptr == end) {
    return result;
  }

  return std::nullopt;
}

template <typename T>  // TODO: add requires T primitive type
  requires std::is_floating_point_v<T>
[[nodiscard]] auto stringToD(const std::string& str) -> std::optional<T> {
  T result{};
  const auto* begin = str.data();
  const auto* end = begin + str.size();  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

  auto [ptr, ec] = std::from_chars(begin, end, result);

  if (ec == std::errc() && ptr == end) {
    return result;
  }

  return std::nullopt;
}

}  // namespace

class InfluxDBSink final : public ITelemetrySink {
public:
  explicit InfluxDBSink(InfluxDBSinkConfig config);
  ~InfluxDBSink() override = default;

  void send(const LogEntry& log_entry) override;

private:
  InfluxDBSinkConfig config_;
  std::unique_ptr<influxdb::InfluxDB> influxdb_;
};

InfluxDBSink::InfluxDBSink(InfluxDBSinkConfig config) : config_(std::move(config)) {
  static constexpr auto URI_FORMAT = "http://{}@{}?db={}";
  const auto url = fmt::format(URI_FORMAT, config_.token, config_.url, config_.database);
  fmt::println("Connecting to InfluxDB at {}", url);
  influxdb_ = influxdb::InfluxDBFactory::Get(url);
}

void InfluxDBSink::send(const LogEntry& log_entry) {
  auto point = influxdb::Point{ log_entry.component }
                   .addTag("tag", log_entry.tag)
                   .setTimestamp(log_entry.log_timestamp);
  fmt::println("Sending log entry: {}", log_entry.json_values);
  const auto json_obj = nlohmann::json::parse(log_entry.json_values);
  for (const auto& [key, value] : json_obj.items()) {
    fmt::println("key: {}, value: {}", key, value.dump());
    if (const auto value_i = stringTo<int64_t>(value.get<std::string>()); value_i.has_value()) {
      point.addField(key, value_i.value());
    } else if (const auto value_d = stringToD<double>(value.get<std::string>()); value_d.has_value()) {
      point.addField(key, value_d.value());
    } else {
      point.addField(key, value.get<std::string>());
    }
  }

  try {
    influxdb_->write(std::move(point));
  } catch (std::exception& e) {
    LOG(ERROR) << fmt::format("Failed to publish to InfluxDB: {}", e.what());
  }

  // TODO: if response.status_code != 200 -> log error
  // LOG_IF(ERROR, response.status_code != 200)
  //     << fmt::format("Failed to publish to REST endpoint with code {}, reason: {}, message {}",
  //                    response.status_code, response.reason, response.text);
}

auto createInfluxDBSink(InfluxDBSinkConfig config) -> std::unique_ptr<ITelemetrySink> {
  return std::make_unique<InfluxDBSink>(std::move(config));
}

}  // namespace heph::telemetry
