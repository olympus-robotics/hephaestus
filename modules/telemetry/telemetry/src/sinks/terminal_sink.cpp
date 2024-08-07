//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/telemetry/sinks/terminal_sink.h"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/std.h>

using ValueMap = std::unordered_map<std::string, heph::telemetry::Metric::ValueType>;
using ValuePair = std::pair<const std::string, heph::telemetry::Metric::ValueType>;

template <>
struct fmt::formatter<ValuePair> {
  template <typename FormatParseContext>
  constexpr static auto parse(FormatParseContext& ctx) {
    return ctx.end();
  }

  constexpr static auto format(const ValuePair& value, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "\n  {}: {}", value.first, value.second);
  }
};

template <>
struct fmt::formatter<ValueMap> {
  template <typename FormatParseContext>
  constexpr static auto parse(FormatParseContext& ctx) {
    return ctx.end();
  }

  static auto format(const ValueMap& value, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "{}", fmt::join(value.begin(), value.end(), ""));
  }
};

namespace heph::telemetry::sinks {

void TerminalMetricSink::send(const heph::telemetry::Metric& metric) {
  fmt::println("[Metrics][{}] [{}] tag: {}, id: {}{}", metric.timestamp, metric.component, metric.tag,
               metric.id, metric.values);
};
}  // namespace heph::telemetry::sinks
