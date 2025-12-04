//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <fmt/base.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/std.h>  // NOLINT(misc-include-cleaner)

namespace heph {
///@brief Forward declaration of LogLevel. Definition is in log.h, since we only want to include that when
/// logging.
enum LogLevel : std::uint8_t { TRACE, DEBUG, INFO, WARN, ERROR };
}  // namespace heph

namespace heph::telemetry {
template <typename T>
concept NonQuotable = !std::convertible_to<T, std::string> && fmt::is_formattable<T>::value;

template <typename T>
concept Quotable = std::convertible_to<T, std::string>;

template <typename T>
struct Field final {
  std::string key;
  T value;
};

///@brief Wrapper around string literals to enhance them with a location.
///       Note that the message is not owned by this class.
///       We need to use a const char* here in order to enable implicit conversion from
///       `log(LogLevel::INFO,"my string");`. The standard guarantees that string literals exist for the
///       entirety of the program lifetime, so it is fine to use it as `MessageWithLocation("my message");`
struct MessageWithLocation final {
  ///@brief Constructor for interface as `log(Level::Warn, "msg");`
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  MessageWithLocation(const char* s, const std::source_location& l = std::source_location::current())
    : value(s), location(l) {
  }

  ///@brief Constructor for interface as `log(Level::Warn, "msg"s);` or `log(Level::Warn, std::format(...));`
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  MessageWithLocation(std::string s, const std::source_location& l = std::source_location::current())
    : value(std::move(s)), location(l) {
  }

  std::string value;
  std::source_location location;
};

/// @brief A class that allows easy composition of logs for structured logging.
///       Example(see also log.cpp):
///       ```cpp
///         namespace ht=heph::telemetry;
///         log(LogLevel::INFO, "adding", "speed", 31.3, "tag", "test");
///       ```
///         logs
///        'level=info hostname=goofy location=log.h:123 thread-id=5124 time=2023-12-3T8:52:02+0
/// module="my module" message="adding" id=12345 speed=31.3 tag="test"'
///        Remark: We would like to use libfmt with quoted formatting as proposed in
///                https://github.com/fmtlib/fmt/issues/825 once its included (instead of sstream).
struct LogEntry {
  using FieldsT = std::vector<Field<std::string>>;
  using ClockT = std::chrono::system_clock;

  LogEntry(LogLevel level, MessageWithLocation&& message);

  /// @brief General loginfo consumer, should be used like LogEntry("my message") << Field{"field", 1234}
  ///        Converted to string with stringstream.
  ///
  /// @tparam T any type that is consumeable by stringstream
  /// @param key identifier for the logging data
  /// @param value value of the logging data
  /// @return LogEntry&&

  template <NonQuotable T>
  auto operator<<(const Field<T>& field) -> LogEntry&& {
    fields.emplace_back(field.key, fmt::format("{}", field.value));

    return std::move(*this);
  }

  /// @brief Specialized loginfo consumer for string types so that they are quoted, should be used like
  ///        LogEntry("my message") << Field{"field", "mystring"}
  ///
  /// @tparam T any type that is consumeable by stringstream
  /// @param key identifier for the logging data
  /// @param val value of the logging data
  /// @return LogEntry&&

  template <Quotable S>
  auto operator<<(const Field<S>& field) -> LogEntry&& {
    fields.emplace_back(field.key, fmt::format("{:?}", field.value));

    return std::move(*this);
  }

  LogLevel level;
  std::string message;
  std::source_location location;
  std::thread::id thread_id;
  ClockT::time_point time;
  std::string hostname;
  std::string module;

  FieldsT fields;
};

/// @brief Interface for sinks that log entries can be sent to.
struct ILogSink {
  using Formatter = std::function<std::string(const LogEntry& log)>;

  virtual ~ILogSink() = default;

  /// @brief Main interface to the sink, gets called on log.
  ///
  /// @param log_entry
  virtual void send(const LogEntry& log_entry) = 0;
};
}  // namespace heph::telemetry

template <>
struct fmt::formatter<heph::telemetry::Field<std::string>> : fmt::formatter<std::string_view> {
  static auto format(const heph::telemetry::Field<std::string>& field, fmt::format_context& ctx) {
    return fmt::format_to(ctx.out(), "{}={}", field.key, field.value);
  }
};

template <>
struct fmt::formatter<heph::LogLevel> : fmt::formatter<std::string_view> {
  static auto format(const heph::LogLevel& level, fmt::format_context& ctx) {
    const auto* const level_name = [&] {
      switch (level) {
        case heph::LogLevel::TRACE:
          return "TRACE";
        case heph::LogLevel::DEBUG:
          return "DEBUG";
        case heph::LogLevel::INFO:
          return "INFO";
        case heph::LogLevel::WARN:
          return "WARN";
        case heph::LogLevel::ERROR:
          return "ERROR";
      }
      return "UNKNOWN";
    }();

    return fmt::format_to(ctx.out(), "{}", level_name);
  }
};

/// @brief Formatter for LogEntry adhering to logfmt rules.
template <>
struct fmt::formatter<heph::telemetry::LogEntry> : fmt::formatter<std::string_view> {
  static auto format(const heph::telemetry::LogEntry& log, fmt::format_context& ctx) {
    return fmt::format_to(
        ctx.out(),
        "level={} hostname={:?} location=\"{}:{}\" thread-id={} time={:%Y-%m-%dT%H:%M:%SZ} "
        "module={}, message={:?} {}",
        log.level, log.hostname, std::filesystem::path{ log.location.file_name() }.filename().string(),
        log.location.line(), log.thread_id, log.time, log.module, log.message, fmt::join(log.fields, " "));
  }
};
