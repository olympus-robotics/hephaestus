//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <source_location>
#include <string>
#include <utility>

#include <fmt/base.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>

namespace heph {

namespace error_handling::detail {
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

void panicImpl(const std::source_location& location, const std::string& formatted_message);

}  // namespace error_handling::detail

/// @brief  User function to panic. Internally this throws a Panic exception.
/// > Note: If macro `DISABLE_EXCEPTIONS` is defined, this function terminates printing the message. In this
/// case, the whole code should be considered noexcept.
///
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename... Args>
void panic(error_handling::detail::StringLiteralWithLocation<Args...> message, Args&&... args) {
  auto formatted_message = fmt::format(message.value, std::forward<Args>(args)...);
  error_handling::detail::panicImpl(message.location, formatted_message);
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
