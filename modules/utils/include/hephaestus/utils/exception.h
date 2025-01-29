//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#pragma once

#include <source_location>
#include <stdexcept>
#include <string>
#include <type_traits>

#ifdef DISABLE_EXCEPTIONS
#include <absl/log/absl_check.h>
#include <fmt/format.h>
#endif

namespace heph {

//=================================================================================================
/// Base class for exceptions
/// @include exception_example.cpp
class Exception : public std::runtime_error {
public:
  /// @param message A message describing the error and what caused it
  /// @param location Location in the source where the error was triggered at
  explicit Exception(const std::string& message, std::source_location location);
};

/// @brief  User function to throw an exception.
/// > Note: If macro `DISABLE_EXCEPTIONS` is defined, this function terminates printing the message. In this
/// case, the whole code should be considered noexcept.
///
/// @tparam T Exception type, derived from heph::Exception
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename T>
  requires std::is_base_of_v<Exception, T>
constexpr void throwException(const std::string& message,
                              std::source_location location = std::source_location::current()) {
#ifndef DISABLE_EXCEPTIONS
  throw T{ message, location };
#else
  auto e = T{ message, location };
  ABSL_CHECK(false) << fmt::format("[ERROR {}] {} at {}:{}", e.what(), message, location.file_name(),
                                   location.line());
#endif
}

/// @brief  User function to throw a conditional exception. The whole code should be considered noexcept. Will
/// use CHECK if DISABLE_EXCEPTIONS = ON
/// @tparam T Exception type, derived from heph::Exception
/// @param condition Condition whether to throw the exception
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename T>
  requires std::is_base_of_v<Exception, T>
constexpr void throwExceptionIf(bool condition, const std::string& message,
                                std::source_location location = std::source_location::current()) {
#ifndef DISABLE_EXCEPTIONS
  if (condition) [[unlikely]] {
    throw T{ message, location };
  }
#else
  auto e = T{ message, location };
  ABSL_CHECK(!condition) << fmt::format("[ERROR {}] {} at {}:{}", e.what(), message, location.file_name(),
                                        location.line());
#endif
}

#ifdef DISABLE_EXCEPTIONS
#define EXPECT_THROW_OR_DEATH(statement, expected_exception, expected_matcher)                               \
  EXPECT_DEATH(statement, expected_matcher)
#else
#define EXPECT_THROW_OR_DEATH(statement, expected_exception, expected_matcher)                               \
  EXPECT_THROW(statement, expected_exception)
#endif

//=================================================================================================
/// Exception raised on operating with mismatched types. Examples
/// - serialisation/deserialisation across incompatible types
/// - Typecasting between incompatible types
class TypeMismatchException : public heph::Exception {
public:
  TypeMismatchException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to invalid/incomplete/undefined data
class InvalidDataException : public heph::Exception {
public:
  InvalidDataException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to invalid/incomplete/undefined configuration
class InvalidConfigurationException : public heph::Exception {
public:
  InvalidConfigurationException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to invalid parameters
class InvalidParameterException : public heph::Exception {
public:
  InvalidParameterException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to invalid or unsupported operation
class InvalidOperationException : public heph::Exception {
public:
  InvalidOperationException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to a hardware issue
class HardwareException : public heph::Exception {
public:
  HardwareException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due failed Zenoh operation
class FailedZenohOperation : public heph::Exception {
public:
  FailedZenohOperation(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due failed serdes operation
class FailedSerdesOperation : public heph::Exception {
public:
  FailedSerdesOperation(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

}  // namespace heph
