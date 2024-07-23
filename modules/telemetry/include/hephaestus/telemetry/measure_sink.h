//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>
#include <string_view>

namespace heph::telemetry {
using ClockT = std::chrono::system_clock;

struct MeasureEntry {
  std::string component;
  std::string tag;
  ClockT::time_point log_timestamp;
  std::string json_values;
};
[[nodiscard]] auto toJSON(const MeasureEntry& log) -> std::string;
void fromJSON(std::string_view json, MeasureEntry& log);

class IMeasureSink {
public:
  virtual ~IMeasureSink() = default;

  virtual void send(const MeasureEntry& log_entry) = 0;
};

}  // namespace heph::telemetry
