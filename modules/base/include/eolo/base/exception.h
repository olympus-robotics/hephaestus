//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
// MIT License
//=================================================================================================

#pragma once

#include <source_location>
#include <stdexcept>
#include <type_traits>

namespace eolo {

//=================================================================================================
/// Base class for exceptions
/// @include exception_example.cpp
class Exception : public std::runtime_error {
public:
  /// @param message A message describing the error and what caused it
  /// @param location Location in the source where the error was triggered at
  explicit Exception(const std::string& message, std::source_location location);
};

/// @brief  User function to throw an exception
/// @tparam T Exception type, derived from eolo::Exception
/// @param message A message describing the error and what caused it
/// @param location Location in the source where the error was triggered at
template <typename T>
  requires std::is_base_of_v<Exception, T>
constexpr void throwException(const std::string& message,
                              std::source_location location = std::source_location::current()) {
  throw T{ message, location };
}

//=================================================================================================
/// Exception raised on operating with mismatched types. Examples
/// - serialisation/deserialisation across incompatible types
/// - Typecasting between incompatible types
class TypeMismatchException : public eolo::Exception {
public:
  TypeMismatchException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to invalid/incomplete/undefined configuration
class InvalidConfigurationException : public eolo::Exception {
public:
  InvalidConfigurationException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to invalid parameters
class InvalidParameterException : public eolo::Exception {
public:
  InvalidParameterException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

//=================================================================================================
/// Exception raised due to invalid or unsupported operation
class InvalidOperationException : public eolo::Exception {
public:
  InvalidOperationException(const std::string& msg, std::source_location loc) : Exception(msg, loc) {
  }
};

}  // namespace eolo
