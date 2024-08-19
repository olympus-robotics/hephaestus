//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/metric_record.h"

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
#include <fmt/core.h>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/telemetry/metric_sink.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::telemetry {

namespace internal {
namespace {
[[nodiscard]] auto jsonToValue(const nlohmann::json& json_value) -> std::optional<Metric::ValueType> {
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
    if (const auto value_i = utils::string::stringTo<int64_t>(value_str); value_i.has_value()) {
      return value_i.value();
    }

    return value_str;
  }

  return {};
}

void jsonToValues(const nlohmann::json& json, std::unordered_map<std::string, Metric::ValueType>& values,
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

auto jsonToValuesMap(std::string_view json) -> std::unordered_map<std::string, Metric::ValueType> {
  const auto json_obj = nlohmann::json::parse(json);
  std::unordered_map<std::string, Metric::ValueType> values;

  jsonToValues(json_obj, values, "");

  return values;
}
}  // namespace internal

class MetricRecorder {
public:
  explicit MetricRecorder();
  ~MetricRecorder();
  MetricRecorder(const MetricRecorder&) = delete;
  MetricRecorder(MetricRecorder&&) = delete;
  auto operator=(const MetricRecorder&) -> MetricRecorder& = delete;
  auto operator=(MetricRecorder&&) -> MetricRecorder& = delete;

  static void registerSink(std::unique_ptr<IMetricSink> sink);

  static void record(const Metric& metric);

private:
  [[nodiscard]] static auto instance() -> MetricRecorder&;
  void processEntries(const Metric& entry);

private:
  std::mutex sink_mutex_;
  std::vector<std::unique_ptr<IMetricSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  concurrency::MessageQueueConsumer<Metric> entries_consumer_;
};

void registerMetricSink(std::unique_ptr<IMetricSink> sink) {
  MetricRecorder::registerSink(std::move(sink));
}

void record(const Metric& metric) {
  MetricRecorder::record(metric);
}

MetricRecorder::MetricRecorder()
  : entries_consumer_([this](const Metric& entry) { processEntries(entry); }, std::nullopt) {
  entries_consumer_.start();
}

MetricRecorder::~MetricRecorder() {
  entries_consumer_.stop();
}

auto MetricRecorder::instance() -> MetricRecorder& {
  static MetricRecorder telemetry;
  return telemetry;
}

void MetricRecorder::registerSink(std::unique_ptr<IMetricSink> sink) {
  auto& telemetry = instance();
  const std::lock_guard lock(telemetry.sink_mutex_);
  telemetry.sinks_.push_back(std::move(sink));
}

void MetricRecorder::record(const Metric& metric) {
  auto& telemetry = instance();
  telemetry.entries_consumer_.queue().forcePush(metric);
}

void MetricRecorder::processEntries(const Metric& entry) {
  const std::lock_guard lock(sink_mutex_);
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

}  // namespace heph::telemetry
