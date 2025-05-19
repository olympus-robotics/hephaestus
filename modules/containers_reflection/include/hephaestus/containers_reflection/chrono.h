//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>

#include <fmt/format.h>
#include <rfl.hpp>  // NOLINT(misc-include-cleaner)

#include "hephaestus/telemetry/log.h"
#include "hephaestus/utils/exception.h"

namespace rfl {

/// \brief Specialization of the Reflector for chrono based Duration type.
template <typename Rep, typename Period>
struct Reflector<std::chrono::duration<Rep, Period>> {  // NOLINT(misc-include-cleaner)
  using ReflType = std::string;

  static auto from(const std::chrono::duration<Rep, Period>& x) noexcept -> ReflType {
    return fmt::format("{:.6f}s", std::chrono::duration_cast<std::chrono::duration<float>>(x).count());
  }

  static auto to(const ReflType& value) -> std::chrono::duration<Rep, Period> {
    heph::panicIf(value.empty(), "Duration string is empty.");
    std::string numeric_part;
    // Check if the string ends with 's'
    if (value.back() == 's') {
      // Extract the numeric part by removing the 's' at the end
      numeric_part = value.substr(0, value.length() - 1);
    } else {
      heph::log(heph::ERROR,
                "Duration string does not end with 's'. Expected format like '123.456s' continuing rest of "
                "the number",
                "full string", value);
      numeric_part = value;
    }

    // Note that those invalid casts may throw in which case reflect-cpp will return an error
    const float value_in_seconds = std::stof(numeric_part);
    const std::chrono::duration<float> duration_sec(value_in_seconds);

    return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(duration_sec);
  }
};
}  // namespace rfl
