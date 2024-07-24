//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>

namespace heph::telemetry {
using ClockT = std::chrono::system_clock;

struct DataPoint {
  [[nodiscard]] auto operator==(const DataPoint&) const -> bool = default;

  std::string component;  ///< The component that is logging the data_point_record, e.g. SLAM, Navigation,
                          ///< etc.
  std::string tag;        ///< The tag of the data_point_record used to identify who created it, e.g.
                          ///< "front_camera", "motor1", etc.
  ClockT::time_point timestamp;

  using ValueType = std::variant<int64_t, double, std::string, bool>;
  std::unordered_map<std::string, ValueType> values;
};

class IDataPointSink {
public:
  virtual ~IDataPointSink() = default;

  virtual void send(const DataPoint&) = 0;
};

}  // namespace heph::telemetry
