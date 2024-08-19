//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/metric_sinks/terminal_sink.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/std.h>  // NOLINT(misc-include-cleaner)

#include "hephaestus/telemetry/metric_sink.h"

using ValueMap = std::unordered_map<std::string, heph::telemetry::Metric::ValueType>;
using ValuePair = std::pair<const std::string, heph::telemetry::Metric::ValueType>;

template <>
struct fmt::formatter<ValuePair> : fmt::formatter<std::string_view> {
  static auto format(const ValuePair& value, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "{}: {}", value.first, value.second);
  }
};

template <>
struct fmt::formatter<ValueMap> : fmt::formatter<std::string_view> {
  static auto format(const ValueMap& value, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "\n\t{}", fmt::join(value.begin(), value.end(), "\n\t"));
  }
};

namespace heph::telemetry::metric_sinks {

void TerminalMetricSink::send(const Metric& metric) {
  fmt::println("[Metrics][{}] [{}] tag: {}, id: {}{}", metric.timestamp, metric.component, metric.tag,
               metric.id, metric.values);
};
}  // namespace heph::telemetry::metric_sinks
