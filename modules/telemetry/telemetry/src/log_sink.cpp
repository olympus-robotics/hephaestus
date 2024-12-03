//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/telemetry/log_sink.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <fmt/base.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>  // NOLINT(misc-include-cleaner)

#include "hephaestus/utils/utils.h"

template <>
struct fmt::formatter<heph::telemetry::Field<std::string>> : fmt::formatter<std::string_view> {
  static auto format(const heph::telemetry::Field<std::string>& field, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "{}={}", field.key, field.value);
  }
};

template <>
struct fmt::formatter<heph::LogLevel> : fmt::formatter<std::string_view> {
  static auto format(const heph::LogLevel& level, fmt::format_context& ctx) {
    switch (level) {
      case heph::LogLevel::TRACE:
        return fmt::format_to(ctx.out(), "trace");
      case heph::LogLevel::DEBUG:
        return fmt::format_to(ctx.out(), "debug");
      case heph::LogLevel::INFO:
        return fmt::format_to(ctx.out(), "info");
      case heph::LogLevel::WARN:
        return fmt::format_to(ctx.out(), "warn");
      case heph::LogLevel::ERROR:
        return fmt::format_to(ctx.out(), "error");
      case heph::LogLevel::FATAL:
        return fmt::format_to(ctx.out(), "fatal");
    }
  }
};

namespace heph {
auto operator<<(std::ostream& os, const LogLevel& level) -> std::ostream& {
  switch (level) {
    case LogLevel::TRACE:
      os << "trace";
      break;
    case LogLevel::DEBUG:
      os << "debug";
      break;
    case LogLevel::INFO:
      os << "info";
      break;
    case LogLevel::WARN:
      os << "warn";
      break;
    case LogLevel::ERROR:
      os << "error";
      break;
    case LogLevel::FATAL:
      os << "fatal";
      break;
  }

  return os;
}
}  // namespace heph

namespace heph::telemetry {
// Instead of deactvating the following check we could globally set AllowPartialMove to true
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
LogEntry::LogEntry(LogLevel level_in, MessageWithLocation&& message_in)
  : level{ level_in }
  , message{ std::move(message_in.value) }
  , location{ message_in.location }
  , thread_id{ std::this_thread::get_id() }
  , time{ LogEntry::ClockT::now() }
  , hostname{ heph::utils::getHostName() } {
}

auto format(const LogEntry& log) -> std::string {
  return fmt::format(
      "level={} hostname={:?} location=\"{}:{}\" thread-id={} time={:%Y-%m-%dT%H:%M:%SZ} message={:?} {}",
      log.level, log.hostname, std::filesystem::path{ log.location.file_name() }.filename().string(),
      log.location.line(), log.thread_id, log.time, log.message, fmt::join(log.fields, " "));
}

}  // namespace heph::telemetry
