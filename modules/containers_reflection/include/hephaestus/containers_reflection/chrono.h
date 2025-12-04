//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>

#include <fmt/format.h>
#include <rfl.hpp>  // NOLINT(misc-include-cleaner)
#include <rfl/internal/has_reflector.hpp>

namespace rfl {

/// \brief Specialization of the Reflector for chrono based Duration type.
template <typename Rep, typename Period>
struct Reflector<std::chrono::duration<Rep, Period>> {  // NOLINT(misc-include-cleaner)
  using ReflType = std::string;

  static auto from(const std::chrono::duration<Rep, Period>& x) noexcept -> ReflType {
    return fmt::format("{:.6f}s", std::chrono::duration_cast<std::chrono::duration<double>>(x).count());
  }

  static auto to(const ReflType& value) -> std::chrono::duration<Rep, Period> {
    // NOTE: in this function we throw exceptions in case of errors, as reflect-cpp expects that.
    if (value.empty()) {
      throw std::invalid_argument("Duration string is empty.");
    }
    if (value.back() != 's') {
      throw std::invalid_argument(fmt::format(
          "Duration string does not end with 's'. Expected format like '123.456231s', got '{}'.", value));
    }

    // Note that those invalid casts may throw in which case reflect-cpp will return an error
    const size_t parsed_length = value.length() - 1;  // Remove the 's' at the end
    double value_in_seconds = 0.0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto [ptr, err] = std::from_chars(value.data(), value.data() + parsed_length, value_in_seconds,
                                            std::chars_format::general);
    if (err != std::errc()) {
      throw std::invalid_argument(fmt::format("Error parsing duration string: {}", value));
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (ptr != value.data() + parsed_length) {
      throw std::invalid_argument(fmt::format("Only a part of the string {} was parsed.", value));
    }

    const std::chrono::duration<double> duration_sec(value_in_seconds);

    return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(duration_sec);
  }
};
}  // namespace rfl
