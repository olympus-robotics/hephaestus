//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>

namespace heph::telemetry {
using ClockT = std::chrono::system_clock;

struct LogEntry {
  std::string component;
  std::string tag;
  ClockT::time_point log_timestamp;
  std::string json_values;
};

class ITelemetrySink {
public:
  virtual ~ITelemetrySink() = default;

  virtual void send(const LogEntry& log_entry) = 0;
};

/// Create a sink that print logs to the terminal.
[[nodiscard]] auto createTerminalSink() -> std::unique_ptr<ITelemetrySink>;

struct RESTSinkConfig {
  std::string url;
};
[[nodiscard]] auto createRESTSink(RESTSinkConfig config) -> std::unique_ptr<ITelemetrySink>;

struct InfluxDBSinkConfig {
  std::string url;
  std::string token;
  std::string database;
};
[[nodiscard]] auto createInfluxDBSink(InfluxDBSinkConfig config) -> std::unique_ptr<ITelemetrySink>;

}  // namespace heph::telemetry
