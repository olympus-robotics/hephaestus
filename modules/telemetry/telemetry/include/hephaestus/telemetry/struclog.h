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

namespace heph::telemetry {

enum class Level : std::uint8_t { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };
auto operator<<(std::ostream& /*os*/, const Level& /*level*/) -> std::ostream&;

template <typename T>
concept NotQuotable = !std::convertible_to<T, std::string> && requires(std::ostream& os, T t) {
  { os << t } -> std::same_as<std::ostream&>;
};

template <typename T>
concept Quotable = std::convertible_to<T, std::string>;

template <typename T>
struct Field final {
  std::string key;
  T val;
};

/// @brief A class that allows easy composition of logs for structured logging.
///       Example(see also struclog.cpp):
///       ```cpp
///         namespace ht=heph::telemetry;
///         log(ht::LogEntry{Level::INFO, "my module", "adding"} | "id"_f(12345) | ht::Field{"tag", "test"});
///       ```
///         logs
///        'level=info hostname=goofy location=struclog.h:123 thread-id=5124 time=2023-12-3T8:52:02+0
/// module="my module" message="adding" id=12345 tag="test"'
///        Remark: We would like to use libfmt with quoted formatting as proposed in
///                https://github.com/fmtlib/fmt/issues/825 once its included (instead of sstream).
struct LogEntry {
  using FieldsT = std::vector<Field<std::string>>;
  using ClockT = std::chrono::system_clock;

  LogEntry(Level level, std::string&& module, std::string&& message,
           std::source_location location = std::source_location::current());


  /// @brief General loginfo consumer, should be used like LogEntry("my message") | Field{"field", 1234}
  ///        Converted to string with stringstream.
  ///
  /// @tparam T any type that is consumeable by stringstream
  /// @param key identifier for the logging data
  /// @param val value of the logging data
  /// @return LogEntry&&

  template <NotQuotable T>
  auto operator|(const Field<T>& field) -> LogEntry&& {
    std::stringstream ss;
    ss << field.val;
    fields.emplace_back(field.key, ss.str());

    return std::move(*this);
  }

  /// @brief Specialized loginfo consumer for string types so that they are quoted, should be used like
  ///        LogEntry("my message") | Field{"field", "mystring"}
  ///
  /// @tparam T any type that is consumeable by stringstream
  /// @param key identifier for the logging data
  /// @param val value of the logging data
  /// @return LogEntry&&

  template <Quotable S>
  auto operator|(const Field<S>& field) -> LogEntry&& {
    std::stringstream ss;
    // Pointer decay is wanted here to catch char[n] messages
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
    ss << std::quoted(field.val);
    fields.emplace_back(field.key, ss.str());

    return std::move(*this);
  }

  Level level;
  std::string module;
  std::string message;
  std::source_location location;
  std::thread::id thread_id;
  ClockT::time_point time;
  std::string hostname;

  FieldsT fields;
};

/// @brief format function from LogEntry to string. Currently its in logfmt.
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

void log(const LogEntry& log_entry);

void registerLogSink(std::unique_ptr<ILogSink> sink);

}  // namespace heph::telemetry

namespace heph::telemetry::literals {

/// @brief User-defined string literal to enable `Field f="data"_f(mydata);`
///        Hence we can use shorter log calls like `log(LogEntry{Severity::Info, "msg"} | "num"_f(1234));`
constexpr auto operator""_f(const char* key, [[maybe_unused]] size_t _) {
  return [key](auto&& value) { return heph::telemetry::Field{ key, std::forward<decltype(value)>(value) }; };
}
}  // namespace heph::telemetry::literals
