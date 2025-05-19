//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <string>
#include <system_error>

#include <fmt/format.h>
#include <rfl.hpp>  // NOLINT(misc-include-cleaner)

#include "hephaestus/utils/exception.h"

namespace rfl {

/// \brief Specialization of the Reflector for chrono based Duration type.
template <typename Rep, typename Period>
struct Reflector<std::chrono::duration<Rep, Period>> {  // NOLINT(misc-include-cleaner)
  using ReflType = std::string;

  static auto from(const std::chrono::duration<Rep, Period>& x) noexcept -> ReflType {
    return fmt::format("{:.6f}s", std::chrono::duration_cast<std::chrono::duration<double>>(x).count());
  }

  static auto to(const ReflType& value) -> std::chrono::duration<Rep, Period> {
    heph::panicIf(value.empty(), "Duration string is empty.");
    heph::panicIf(
        value.back() != 's',
        fmt::format("Duration string does not end with 's'. Expected format like '123.456231s', got '{}'.",
                    value));
    // Note that those invalid casts may throw in which case reflect-cpp will return an error
    const size_t parsed_length = value.length() - 1;  // Remove the 's' at the end
    double value_in_seconds = 0.0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto [ptr, err] = std::from_chars(value.data(), value.data() + parsed_length, value_in_seconds);
    heph::panicIf(err != std::errc(), fmt::format("Error parsing duration string: {}", value));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    heph::panicIf(ptr != value.data() + parsed_length,
                  fmt::format("Only a part of the string {} was parsed.", value));

    const std::chrono::duration<double> duration_sec(value_in_seconds);

    return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(duration_sec);
  }
};
}  // namespace rfl
