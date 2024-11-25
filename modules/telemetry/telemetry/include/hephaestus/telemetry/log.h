//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "hephaestus/telemetry/log_sink.h"

namespace heph::telemetry {

void registerLogSink(std::unique_ptr<ILogSink> sink);

namespace internal {
void log(LogEntry&& log_entry);

#if (__GNUC__ >= 14) || defined(__clang__)
// NOLINTBEGIN(cppcoreguidelines-rvalue-reference-param-not-moved, misc-unused-parameters,
// cppcoreguidelines-missing-std-forward)
///@brief Stop function for recursion: Uneven number of parameters.
///       This is exists for better understandable compiler errors.
///       Code gcc<14 will still work as expected but the compiler error message is hard to read.
template <typename First>
void logWithFields(LogEntry&&, First&&) {
  static_assert(false, "number of input parameters is uneven.");
}
// NOLINTEND(cppcoreguidelines-rvalue-reference-param-not-moved, misc-unused-parameters,
// cppcoreguidelines-missing-std-forward)
#endif

///@brief Stop function for recursion: Even number of parameters
template <typename First, typename Second>
void logWithFields(LogEntry&& entry, First&& first, Second&& second) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  entry << Field{ .key = std::forward<First>(first), .value = std::forward<Second>(second) };
  log(std::move(entry));
}

///@brief Add fields pairwise to entry. `addFields(entry, "a", 3,"b","lala")` will result in fields a=3 and
/// b="lala".
template <typename First, typename Second, typename... Rest>
void logWithFields(LogEntry&& entry, First&& first, Second&& second, Rest&&... rest) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  entry << Field{ .key = std::forward<First>(first), .value = std::forward<Second>(second) };
  logWithFields(std::move(entry), std::forward<Rest>(rest)...);
}
}  // namespace internal
}  // namespace heph::telemetry

namespace heph {
///@brief Log a message. Example:
///       ```
///       heph::log(heph::LogLevel::WARN, "speed is over limit", "current_speed", 31.3, "limit", 30.0,
///       "entity", "km/h")
///       ```
template <typename... Args>
void log(LogLevel level, telemetry::MessageWithLocation msg, Args&&... fields) {
  telemetry::internal::logWithFields(telemetry::LogEntry{ level, msg }, std::forward<Args>(fields)...);
}

///@brief Log a message without fields. Example:
///       ```
///       heph::log(heph::LogLevel::WARN, "speed is over limit", "current_speed"))
///       ```
template <typename... Args>
void log(LogLevel level, telemetry::MessageWithLocation msg) {
  telemetry::internal::log(telemetry::LogEntry{ level, msg });
}

}  // namespace heph
