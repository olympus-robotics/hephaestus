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

  std::string component;  ///< The component that is logging the measure, e.g. SLAM, Navigation, etc.
  std::string tag;        ///< The tag of the measure used to identify who created it, e.g. "front_camera",
                          ///< "motor1", etc.
  ClockT::time_point measure_timestamp;
  std::string json_values;
};

class IMeasureSink {
public:
  virtual ~IMeasureSink() = default;

  virtual void send(const MeasureEntry&) = 0;
};

}  // namespace heph::telemetry
