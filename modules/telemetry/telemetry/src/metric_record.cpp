//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/metric_record.h"

#include <cstdint>
#include <exception>
#include <future>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/log/log.h>
#include <absl/strings/numbers.h>
#include <absl/synchronization/mutex.h>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/containers/blocking_queue.h"
#include "hephaestus/telemetry/metric_sink.h"

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
    int64_t value{};
    if (const auto success = absl::SimpleAtoi(value_str, &value); success) {
      return value;
    }

    return value_str;
  }

  if (json_value.is_null()) {
    return std::numeric_limits<double>::quiet_NaN();
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
  void processEntry(const Metric& entry);

  /// @brief Empty the queue so that remaining messages get processed
  void emptyQueue();

private:
  absl::Mutex sink_mutex_;
  std::vector<std::unique_ptr<IMetricSink>> sinks_ ABSL_GUARDED_BY(sink_mutex_);
  containers::BlockingQueue<Metric> entries_;
  std::future<void> message_process_future_;
};

void registerMetricSink(std::unique_ptr<IMetricSink> sink) {
  MetricRecorder::registerSink(std::move(sink));
}

void record(const Metric& metric) {
  MetricRecorder::record(metric);
}

MetricRecorder::MetricRecorder() : entries_{ std::nullopt } {
  message_process_future_ = std::async(std::launch::async, [this]() {
    while (true) {
      auto message = entries_.waitAndPop();
      if (!message.has_value()) {
        emptyQueue();
        return;
      }

      processEntry(message.value());
    }
  });
}

MetricRecorder::~MetricRecorder() {
  try {
    entries_.stop();
    message_process_future_.get();
  } catch (const std::exception& ex) {
    LOG(FATAL) << "While emptying message consumer, exception happened: " << ex.what();
  }
}

auto MetricRecorder::instance() -> MetricRecorder& {
  static MetricRecorder telemetry;
  return telemetry;
}

void MetricRecorder::registerSink(std::unique_ptr<IMetricSink> sink) {
  auto& telemetry = instance();
  const absl::MutexLock lock{ &telemetry.sink_mutex_ };
  telemetry.sinks_.push_back(std::move(sink));
}

void MetricRecorder::record(const Metric& metric) {
  auto& telemetry = instance();
  telemetry.entries_.forcePush(metric);
}

void MetricRecorder::processEntry(const Metric& entry) {
  const absl::MutexLock lock{ &sink_mutex_ };
  for (auto& sink : sinks_) {
    sink->send(entry);
  }
}

void MetricRecorder::emptyQueue() {
  while (!entries_.empty()) {
    auto message = entries_.tryPop();
    if (!message.has_value()) {
      return;
    }

    processEntry(message.value());
  }
}

}  // namespace heph::telemetry
