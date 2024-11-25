#pragma once

#include <functional>
#include <iomanip>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace heph {
enum class LogLevel : std::uint8_t { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };
auto operator<<(std::ostream& /*os*/, const LogLevel& /*log_level*/) -> std::ostream&;
}  // namespace heph

namespace heph::telemetry {
template <typename T>
concept NonQuotable = !std::convertible_to<T, std::string> && requires(std::ostream& os, T t) {
  { os << t } -> std::same_as<std::ostream&>;
};

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
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  MessageWithLocation(const char* s, const std::source_location& l = std::source_location::current())
    : value(s), location(l) {
  }

  const char* value;
  std::source_location location;
};

/// @brief A class that allows easy composition of logs for structured logging.
///       Example(see also struclog.cpp):
///       ```cpp
///         namespace ht=heph::telemetry;
///         log(LogLevel::INFO, "adding", {"speed", 31.3}, {"tag", "test"});
///       ```
///         logs
///        'level=info hostname=goofy location=struclog.h:123 thread-id=5124 time=2023-12-3T8:52:02+0
/// module="my module" message="adding" id=12345 speed=31.3 tag="test"'
///        Remark: We would like to use libfmt with quoted formatting as proposed in
///                https://github.com/fmtlib/fmt/issues/825 once its included (instead of sstream).
struct LogEntry {
  using FieldsT = std::vector<Field<std::string>>;
  using ClockT = std::chrono::system_clock;

  LogEntry(heph::LogLevel level, MessageWithLocation message);

  /// @brief General loginfo consumer, should be used like LogEntry("my message") << Field{"field", 1234}
  ///        Converted to string with stringstream.
  ///
  /// @tparam T any type that is consumeable by stringstream
  /// @param key identifier for the logging data
  /// @param value value of the logging data
  /// @return LogEntry&&

  template <NonQuotable T>
  auto operator<<(const Field<T>& field) -> LogEntry&& {
    std::stringstream ss;
    ss << field.value;
    fields.emplace_back(field.key, ss.str());

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
    std::stringstream ss;
    // Pointer decay is wanted here to catch char[n] messages
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    ss << std::quoted(field.value);
    fields.emplace_back(field.key, ss.str());

    return std::move(*this);
  }

  heph::LogLevel level;
  const char* message;
  std::source_location location;
  std::thread::id thread_id;
  ClockT::time_point time;
  std::string hostname;

  FieldsT fields;
};

/// @brief format function from LogEntry to string adheruing to logfmt rules.
///
/// @param log
/// @return std::string
[[nodiscard]] auto format(const LogEntry& log) -> std::string;

inline auto operator<<(std::ostream& os, const LogEntry& log) -> std::ostream& {
  os << format(log);

  return os;
}

/// @brief Interface for sinks that log entries can be sent to.
struct ILogSink {
  using Formatter = std::function<std::string(const LogEntry& log)>;

  virtual ~ILogSink() = default;

  /// @brief Main interface to the sink, gets called on log.
  ///
  /// @param log_entry
  virtual void send(const LogEntry& log_entry) = 0;
};

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

void registerLogSink(std::unique_ptr<ILogSink> sink);

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
