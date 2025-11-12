//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <exception>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

#include <fmt/base.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>

#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/utils/stack_trace.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph {
namespace error_handling {
namespace detail {
/// @brief  Wrapper around string literals to enhance them with a location.
///         Note that the message is not owned by this class.
///         String literals are used to enable implicit conversion from string literals.
///         The standard guarantees that string literals exist for the entirety of the
///         program lifetime, making it safe to use as `StringLiteralWithLocation("my message")`.
template <typename... Ts>
struct StringLiteralWithLocationImpl final {
  /// @brief Constructor taking a string literal and optional source location
  /// @param s The string literal message
  /// @param l The source location (defaults to current location)
  template <typename S>
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  consteval StringLiteralWithLocationImpl(const S& s,
                                          const std::source_location& l = std::source_location::current())
    : value(s), location(l) {
  }

  fmt::format_string<Ts...> value;  ///< The message string literal
  std::source_location location;    ///< Source code location information

  using impl = StringLiteralWithLocationImpl;
};

template <typename... Ts>
using StringLiteralWithLocation = detail::StringLiteralWithLocationImpl<Ts...>::impl;

}  // namespace detail

/// @brief  Scope guard to enable panic as exceptions within the scope.
/// This is useful for unit tests where we want to catch panics as exceptions.
class PanicAsExceptionScope {
public:
  PanicAsExceptionScope();
  ~PanicAsExceptionScope();

  PanicAsExceptionScope(const PanicAsExceptionScope&) = delete;
  auto operator=(const PanicAsExceptionScope&) -> PanicAsExceptionScope& = delete;
  PanicAsExceptionScope(PanicAsExceptionScope&&) = delete;
  auto operator=(PanicAsExceptionScope&&) -> PanicAsExceptionScope& = delete;
};

/// @brief  Check whether panics should be thrown as exceptions.
[[nodiscard]] auto panicAsException() -> bool;

class PanicException final : public std::runtime_error {
public:
  explicit PanicException(const std::string& message) : std::runtime_error(message) {
  }
};
}  // namespace error_handling

/// @brief  User function to panic. Internally this throws a Panic exception.
/// > Note: If macro `DISABLE_EXCEPTIONS` is defined, this function terminates printing the message. In this
/// case, the whole code should be considered noexcept.
///
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename... Args>
void panic(error_handling::detail::StringLiteralWithLocation<Args...> message, Args&&... args) {
  auto formatted_message = fmt::format(message.value, std::forward<Args>(args)...);
  auto location = std::string(utils::string::truncate(message.location.file_name(), "modules")) + ":" +
                  std::to_string(message.location.line());
  // We want to make sure the log from panic is the last one before termination so we flush before and after
  telemetry::flushLogEntries();
  log(ERROR, "program terminated with panic", "error", std::move(formatted_message), "location",
      std::move(location));
  telemetry::flushLogEntries();

  if (error_handling::panicAsException()) {
    throw error_handling::PanicException(formatted_message);
  }

  std::terminate();
}

/// @brief  User function to panic on condition lazily formatting the message. The whole code should be
/// considered noexcept. Will use CHECK if DISABLE_EXCEPTIONS = ON
/// @param condition Condition whether to panic
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename... Args>
void panicIf(bool condition, error_handling::detail::StringLiteralWithLocation<Args...> message,
             Args&&... args) {
  if (condition) [[unlikely]] {
    panic(std::move(message), std::forward<Args>(args)...);
  }
}

}  // namespace heph
