//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <format>
#include <source_location>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

#ifdef DISABLE_EXCEPTIONS
#include <absl/log/check.h>
#endif

namespace heph {

namespace internal {
///@brief Wrapper around string literals to enhance them with a location.
///       Note that the message is not owned by this class.
///       We need to use a const char* here in order to enable implicit conversion from
///       `log(LogLevel::INFO,"my string");`. The standard guarantees that string literals exist for the
///       entirety of the program lifetime, so it is fine to use it as `MessageWithLocation("my message");`
struct StringLiteralWithLocation final {
  ///@brief Constructor for interface as `log(Level::Warn, "msg");`
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  StringLiteralWithLocation(const char* s, const std::source_location& l = std::source_location::current())
    : value(s), location(l) {
  }

  const char* value;  ///< The message describing the error
  std::source_location location;
};
}  // namespace internal

//=================================================================================================
/// Base class for exceptions
/// @include exception_example.cpp
class Panic : public std::runtime_error {
public:
  /// @param message A message describing the error and what caused it
  /// @param location Location in the source where the error was triggered at
  explicit Panic(std::string message, const std::source_location& location = std::source_location::current());
};

/// @brief  User function to panic. Internally this throws a Panic exception.
/// > Note: If macro `DISABLE_EXCEPTIONS` is defined, this function terminates printing the message. In this
/// case, the whole code should be considered noexcept.
///
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename... Args>
inline void panic(internal::StringLiteralWithLocation message, Args... args) {
  std::string formatted_message;
  if constexpr (sizeof...(args) > 0) {
    formatted_message = fmt::format(fmt::runtime(message.value), std::forward<Args>(args)...);
  } else {
    formatted_message = message.value;
  }
#ifndef DISABLE_EXCEPTIONS
  throw Panic{ std::move(formatted_message), message.location };
#else
  auto e = Panic{ std::move(formatted_message), message.location };
  CHECK(false) << fmt::format("[ERROR {}] at {}:{}", e.what(), message.location.file_name(),
                              message.location.line());
#endif
}

/// @brief  User function to panic on condition. The whole code should be considered noexcept. Will
/// use CHECK if DISABLE_EXCEPTIONS = ON
/// @param condition Condition whether to panic
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename... Args>
inline void panicIf(bool condition, internal::StringLiteralWithLocation message, Args... args) {
  std::string formatted_message;
  if (condition) [[unlikely]] {
    if constexpr (sizeof...(args) > 0) {
      formatted_message = fmt::format(fmt::runtime(message.value), std::forward<Args>(args)...);
    } else {
      formatted_message = message.value;
    }
  }
#ifndef DISABLE_EXCEPTIONS
  if (condition) [[unlikely]] {
    throw Panic{ std::move(formatted_message), message.location };
  }
#else
  auto e = Panic{ std::move(formatted_message), message.location };
  CHECK(!condition) << fmt::format("[ERROR {}] at {}:{}", e.what(), message.location.file_name(),
                                   message.location.line());
#endif
}

/// @brief Macro to test if a statement throws an exception or causes a program death depending
/// on whether the library is compiled with exceptions enabled.
/// @param statement The statement to be tested.
/// @param expected_exception The type of exception expected to be thrown.
/// @param expected_matcher The matcher to be used in case of program death.
/// @note If `DISABLE_EXCEPTIONS` is defined, this macro uses `EXPECT_DEATH` to check for program death.
///       Otherwise, it uses `EXPECT_THROW` to check for the expected exception.
#ifndef DISABLE_EXCEPTIONS
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define EXPECT_THROW_OR_DEATH(statement, expected_exception, expected_matcher)                               \
  EXPECT_THROW(statement, expected_exception)
#else
#define EXPECT_THROW_OR_DEATH(statement, expected_exception, expected_matcher)                               \
  EXPECT_DEATH(statement, expected_matcher)
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif
}  // namespace heph
