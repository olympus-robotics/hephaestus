//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <utility>

#include "hephaestus/telemetry/log_sink.h"

namespace heph::telemetry::internal {
void log(LogEntry&& log_entry) noexcept;

#if (__GNUC__ >= 14) || defined(__clang__)
// NOLINTBEGIN(cppcoreguidelines-rvalue-reference-param-not-moved, misc-unused-parameters,
// cppcoreguidelines-missing-std-forward)
///@brief Stop function for recursion: Uneven number of parameters.
///       This is exists for better understandable compiler errors.
///       Code gcc<14 will still work as expected but the compiler error message is hard to read.
template <typename First>
void logWithFields(LogEntry&&, First&&) noexcept {
  static_assert(false, "number of input parameters is uneven.");
}
// NOLINTEND(cppcoreguidelines-rvalue-reference-param-not-moved, misc-unused-parameters,
// cppcoreguidelines-missing-std-forward)
#endif

///@brief Stop function for recursion: Even number of parameters
template <typename First, typename Second>
void logWithFields(LogEntry&& entry, First&& first, Second&& second) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  entry << Field{ .key = std::forward<First>(first), .value = std::forward<Second>(second) };
  log(std::move(entry));
}

///@brief Add fields pairwise to entry. `addFields(entry, "a", 3,"b","lala")` will result in fields a=3 and
/// b="lala".
template <typename First, typename Second, typename... Rest>
void logWithFields(LogEntry&& entry, First&& first, Second&& second, Rest&&... rest) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  entry << Field{ .key = std::forward<First>(first), .value = std::forward<Second>(second) };
  logWithFields(std::move(entry), std::forward<Rest>(rest)...);
}
}  // namespace heph::telemetry::internal

namespace heph {

///@brief Log a message. Example:
///       ```
///       heph::log(heph::LogLevel::WARN, "speed is over limit", "current_speed", 31.3, "limit", 30.0,
///       "entity", "km/h")
///       ```
template <typename... Args>
void log(LogLevel level, telemetry::MessageWithLocation&& msg, Args&&... fields) noexcept {
  telemetry::internal::logWithFields(telemetry::LogEntry{ level, std::move(msg) },
                                     std::forward<Args>(fields)...);
}

///@brief Log a message without fields. Example:
///       ```
///       heph::log(heph::LogLevel::WARN, "speed is over limit")
///       ```
template <typename... Args>
void log(LogLevel level, telemetry::MessageWithLocation&& msg) noexcept {
  telemetry::internal::log(telemetry::LogEntry{ level, std::move(msg) });
}

///@brief Conditionally log a message. Example:
///       ```
///       heph::log(heph::LogLevel::WARN, a==3,  "speed is over limit", "current_speed", 31.3, "limit", 30.0,
///       "entity", "km/h")
///       ```
template <typename... Args>
void logIf(LogLevel level, bool condition, telemetry::MessageWithLocation&& msg, Args&&... fields) noexcept {
  if (condition) {
    telemetry::internal::logWithFields(telemetry::LogEntry{ level, std::move(msg) },
                                       std::forward<Args>(fields)...);
  }
}

///@brief Conditionally log a message without fields. Example:
///       ```
///       heph::log(heph::LogLevel::WARN, a==3, "speed is over limit")
///       ```
template <typename... Args>
void logIf(LogLevel level, bool condition, telemetry::MessageWithLocation&& msg) noexcept {
  if (condition) {
    telemetry::internal::log(telemetry::LogEntry{ level, std::move(msg) });
  }
}

namespace telemetry {
///@brief Register a sink for logging.
void registerLogSink(std::unique_ptr<telemetry::ILogSink> sink) noexcept;
///@brief Flush all log entries to the sink
void flushLogEntries();
}  // namespace telemetry

}  // namespace heph
