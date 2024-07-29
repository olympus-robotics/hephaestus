//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>

namespace heph::telemetry {
using ClockT = std::chrono::system_clock;

struct Metric {
  [[nodiscard]] auto operator==(const Metric&) const -> bool = default;

  std::string component;  ///< The component that is logging the metric_record, e.g. SLAM, Navigation,
                          ///< etc.
  std::string tag;        ///< The tag of the metric_record used to identify who created it, e.g.
                          ///< "front_camera", "motor1", etc.
  std::size_t id;         ///< The id of this metric. Can be used to match metrics from different components.
  ClockT::time_point timestamp;

  using ValueType = std::variant<int64_t, double, std::string, bool>;
  std::unordered_map<std::string, ValueType> values;
};

class IMetricSink {
public:
  virtual ~IMetricSink() = default;

  virtual void send(const Metric&) = 0;
};

}  // namespace heph::telemetry
