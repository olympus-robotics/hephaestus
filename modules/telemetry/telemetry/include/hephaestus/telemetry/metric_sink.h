//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/base.h>
#include <fmt/format.h>

namespace heph::telemetry {
using ClockT = std::chrono::system_clock;

struct Metric {
  [[nodiscard]] auto operator==(const Metric&) const -> bool = default;
  [[nodiscard]] auto toString() const -> std::string;

  std::string component;  ///< The component that is logging the metric_record, e.g. SLAM, Navigation,
                          ///< etc.
  std::string tag;        ///< The tag of the metric_record used to identify who created it, e.g.
                          ///< "front_camera", "motor1", etc.
  ClockT::time_point timestamp;

  using ValueType = std::variant<int64_t, double, std::string, bool>;
  using KeyValueType = std::pair<std::string, ValueType>;
  std::vector<KeyValueType> values;
};

class IMetricSink {
public:
  virtual ~IMetricSink() = default;

  virtual void send(const Metric&) = 0;
};

}  // namespace heph::telemetry

template <>
struct fmt::formatter<heph::telemetry::Metric> : fmt::formatter<std::string_view> {
  static auto format(const heph::telemetry::Metric& metric, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "{}", metric.toString());
  }
};
