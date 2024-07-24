//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/data_point_record.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/log/log.h>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/telemetry/data_point_sink.h"

namespace heph::telemetry {

namespace internal {
namespace {
[[nodiscard]] auto stringToInt64(const std::string& str) -> std::optional<int64_t> {
  const char* start = str.c_str();
  char* end{};
  errno = 0;

  static constexpr int BASE = 10;
  int64_t result = std::strtoll(start, &end, BASE);

  if (errno != ERANGE && *end == '\0') {
    return result;
  }

  return std::nullopt;
}

[[nodiscard]] auto jsonToValue(const nlohmann::json& json_value) -> std::optional<DataPoint::ValueType> {
  if (json_value.is_boolean()) {
    return json_value.get<bool>();
  }

  if (json_value.is_number_float()) {
    return json_value.get<double>();
  }

  if (json_value.is_number_integer() || json_value.is_number_unsigned()) {
    return json_value.get<int64_t>();
  }

  if (json_value.is_string()) {
    auto value_str = json_value.get<std::string>();
    // According to JSON specification integer bigger than 32bits are converted to string, so we need to check
    // if the string is actually a number.
    if (const auto value_i = stringToInt64(value_str); value_i.has_value()) {
      return value_i.value();
    }

    return value_str;
  }

  return {};
}

void jsonToValues(const nlohmann::json& json, std::unordered_map<std::string, DataPoint::ValueType>& values,
                  std::string_view key_prefix) {
  for (const auto& [key, value] : json.items()) {
    const auto full_key = key_prefix.empty() ? key : fmt::format("{}.{}", key_prefix, key);
    if (const auto value_opt = jsonToValue(value); value_opt.has_value()) {
      values[full_key] = value_opt.value();
    } else if (value.is_object()) {
      // If it's an object we need to recursively call this function.
      // The key_prefix is updated to include the current key.
      // For a nested value the key will be the concatenation of all the keys in the path.
      jsonToValues(value, values, full_key);
    } else {
      // NOTE: we do not support arrays.
      LOG(ERROR) << fmt::format("Failed to parse value for key: {}, value: {}", full_key, value.dump());
    }
  }
}

}  // namespace

auto jsonToValuesMap(std::string_view json) -> std::unordered_map<std::string, DataPoint::ValueType> {
  const auto json_obj = nlohmann::json::parse(json);
  std::unordered_map<std::string, DataPoint::ValueType> values;

  jsonToValues(json_obj, values, "");

  return values;
}
}  // namespace internal

class Probe {
public:
  explicit Probe();
  static void registerDataPointSink(std::unique_ptr<IDataPointSink> sink);

  static void record(const DataPoint& data_point);

private:
  [[nodiscard]] static auto instance() -> Probe&;
  void processMeasureEntries(const DataPoint& entry);

private:
  std::mutex sink_mutex_;
  std::vector<std::unique_ptr<IDataPointSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  concurrency::MessageQueueConsumer<DataPoint> measure_entries_consumer_;
};

void registerDataPointSink(std::unique_ptr<IDataPointSink> sink) {
  Probe::registerDataPointSink(std::move(sink));
}

void record(const DataPoint& data_point) {
  Probe::record(data_point);
}

Probe::Probe()
  : measure_entries_consumer_([this](const DataPoint& entry) { processMeasureEntries(entry); },
                              std::nullopt) {
}

auto Probe::instance() -> Probe& {
  static Probe telemetry;
  return telemetry;
}

void Probe::registerDataPointSink(std::unique_ptr<IDataPointSink> sink) {
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  telemetry.sinks_.push_back(std::move(sink));
}

void Probe::record(const DataPoint& data_point) {
  auto& telemetry = instance();
  telemetry.measure_entries_consumer_.queue().forcePush(data_point);
}

void Probe::processMeasureEntries(const DataPoint& entry) {
  const std::lock_guard lock(sink_mutex_);
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

}  // namespace heph::telemetry
