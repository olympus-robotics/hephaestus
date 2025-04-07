//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry_influxdb_sink/influxdb_metric_sink.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <InfluxDB/InfluxDB.h>
#include <InfluxDB/InfluxDBFactory.h>
#include <InfluxDB/Point.h>
#include <absl/strings/ascii.h>
#include <fmt/format.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/metric_sink.h"

namespace heph::telemetry_sink {
namespace {

[[nodiscard]] auto isNaN(const auto& value) -> bool {
  if constexpr (std::is_same_v<std::decay_t<decltype(value)>, double> ||
                std::is_same_v<std::decay_t<decltype(value)>, int64_t>) {
    return std::isnan(value);
  } else if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>) {
    return absl::AsciiStrToLower(value) == "nan";
  }

  return false;
}

[[nodiscard]] auto createInfluxdbPoint(const telemetry::Metric& entry) -> influxdb::Point {
  auto point = influxdb::Point{ entry.component }.addTag("tag", entry.tag).setTimestamp(entry.timestamp);
  for (const auto& [key, value] : entry.values) {
    std::visit(
        [&point, &key](auto&& arg) {
          if (isNaN(arg)) {
            return;
          }

          // When coming from JSON a value can be erroneously first creed as int and then become double.
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
  heph::log(heph::DEBUG, "connecting to InfluxDB", "url", url);
  influxdb_ = influxdb::InfluxDBFactory::Get(url);

  if (config_.flush_period.has_value()) {
    // NOTE: we define a very large number for the batch size as we want to flush at a fixed rate.
    static constexpr std::size_t DEFAULT_BATCH_SIZE = 1e6;
    influxdb_->batchOf(DEFAULT_BATCH_SIZE);
    spinner_ = std::make_unique<concurrency::Spinner>(
        [this]() {
          try {
            const std::scoped_lock<std::mutex> lock(mutex_);
            influxdb_->flushBatch();
          } catch (std::exception& e) {
            heph::log(heph::ERROR, "failed to flush batch to InfluxDB", "exception", e.what());
          }

          return concurrency::Spinner::SpinResult::CONTINUE;
        },
        config_.flush_period.value());
    spinner_->start();
  } else if (config_.batch_size.has_value()) {
    influxdb_->batchOf(config_.batch_size.value());
  }
}

InfluxDBSink::~InfluxDBSink() {
  if (spinner_) {
    spinner_->stop().get();
  }
}

void InfluxDBSink::send(const telemetry::Metric& entry) {
  auto point = createInfluxdbPoint(entry);

  try {
    const std::scoped_lock<std::mutex> lock(mutex_);
    influxdb_->write(std::move(point));
  } catch (std::exception& e) {
    heph::log(heph::ERROR, "failed to publish to InfluxDB", "exception", e.what());
  }
}

}  // namespace heph::telemetry_sink
