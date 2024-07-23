//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>

namespace heph::telemetry {
using ClockT = std::chrono::system_clock;

struct MeasureEntry {
  [[nodiscard]] auto operator==(const MeasureEntry& other) const -> bool = default;

  std::string component;
  std::string tag;
  ClockT::time_point measure_timestamp;
  std::string json_values;
};

class IMeasureSink {
public:
  virtual ~IMeasureSink() = default;

  virtual void send(const MeasureEntry& measure_entry) = 0;
};

}  // namespace heph::telemetry
