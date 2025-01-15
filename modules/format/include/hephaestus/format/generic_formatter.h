//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================
#pragma once

#include <chrono>
#include <string>
#include <type_traits>

#include <fmt/base.h>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <rfl.hpp>
#include <rfl/yaml.hpp>

#include "hephaestus/utils/concepts.h"

namespace heph::format {

// @brief Custom formatter for various data types using reflect-cpp
///
/// @tparam T The type of data to format.
/// @param data The data to format.
/// @return A formatted string representation of the data.
template <typename T>
auto toString(const T& data) -> std::string {
  return rfl::yaml::write(data);
}

template <typename T>
concept Formattable = requires(std::ostream& os, const T& t) {
  { t.format() } -> std::same_as<std::string>;
};

}  // namespace heph::format

namespace rfl {
/// \brief Specialization of the Reflector for chrono based Timestamp type. See
/// https://github.com/getml/reflect-cpp/blob/main/docs/custom_parser.md
using SystemClockType = std::chrono::system_clock;
template <>
struct Reflector<std::chrono::time_point<SystemClockType>> {
  using ReflType = std::string;

  static auto to(const ReflType& x) noexcept -> std::chrono::time_point<SystemClockType> {
    std::tm tm = {};
    std::stringstream ss(x.data());
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return SystemClockType::from_time_t(std::mktime(&tm));
  }

  static auto from(const std::chrono::time_point<SystemClockType>& x) noexcept -> ReflType {
    return fmt::format("{:%Y-%m-%d %H:%M:%S}",
                       std::chrono::time_point_cast<std::chrono::microseconds, SystemClockType>(x));
  }
};

template <heph::format::Formattable T>
struct Reflector<T> {
  using ReflType = std::string;

  static auto from(const T& x) noexcept -> ReflType {
    return x.format();
  }
};
}  // namespace rfl

template <typename T>
  requires(!std::is_arithmetic_v<T> && !heph::IsStringLike<T> && !fmt::detail::has_to_string_view<T>::value)
struct fmt::formatter<T> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const T& a, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", heph::format::toString(a));
  }
};
