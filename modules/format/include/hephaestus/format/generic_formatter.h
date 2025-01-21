//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================
#pragma once

#include <chrono>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/base.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>
#include <rfl.hpp>       // NOLINT(misc-include-cleaner)
#include <rfl/yaml.hpp>  // NOLINT(misc-include-cleaner)

#include "hephaestus/utils/concepts.h"
#include "hephaestus/utils/format/format.h"

namespace heph::format {

/// @brief Custom formatter for various data types using reflect-cpp
///
/// @tparam T The type of data to format.
/// @param data The data to format.
/// @return A formatted string representation of the data.
template <typename T>
auto toString(const T& data) -> std::string {
  return rfl::yaml::write(data);
}
}  // namespace heph::format

namespace rfl {
/// \brief Specialization of the Reflector for chrono based Timestamp type. See
/// https://github.com/getml/reflect-cpp/blob/main/docs/custom_parser.md
/// For implementation of Reflector::to see commit b1a4eda
template <typename Clock>
  requires(heph::ChronoSteadyClockType<Clock> || heph::ChronoSystemClockType<Clock>)
struct Reflector<std::chrono::time_point<Clock>> {  // NOLINT(misc-include-cleaner)
  using ReflType = std::string;

  static auto from(const std::chrono::time_point<Clock>& x) noexcept -> ReflType {
    return heph::utils::format::toString(x);
  }
};

/// \brief Specialization of the Reflector for chrono based Duration type.
template <typename Rep, typename Period>
struct Reflector<std::chrono::duration<Rep, Period>> {  // NOLINT(misc-include-cleaner)
  using ReflType = std::string;

  static auto from(const std::chrono::duration<Rep, Period>& x) noexcept -> ReflType {
    return fmt::format("{:.3f}s", std::chrono::duration_cast<std::chrono::duration<float>>(x).count());
  }
};

/// \brief Specialization of the Reflector for custom format that is defined as described by Formattable<T>.
template <heph::Formattable T>
struct Reflector<T> {  // NOLINT(misc-include-cleaner)
  using ReflType = std::string;

  static auto from(const T& x) noexcept -> ReflType {
    return x.format();
  }
};
}  // namespace rfl

namespace fmt {
/// \brief Generic fmt::formatter for all types that are not handled by fmt library.
template <typename T, typename Char>
  requires(!std::is_arithmetic_v<T> && !heph::StringLike<T> && !detail::has_to_string_view<T>::value &&
           !heph::TimeType<T>)
struct formatter<T, Char> : formatter<std::string_view, Char> {
  template <typename FormatContext>
  auto format(const T& data, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", heph::format::toString(data));
  }
};
}  // namespace fmt

namespace std {
/// \brief Generic operator<< for all types that are not handled by the standard. Note that here we actually
/// need SFINAE, since concept Streamable would fall in infinite recursion.
// NOLINTNEXTLINE(modernize-use-constraints)
template <typename T, typename = std::enable_if_t<!heph::has_stream_operator<T>::value>>
  requires(!is_arithmetic_v<T> && !heph::StringLike<T>)
auto operator<<(ostream& os, const T& data) -> ostream& {
  return os << heph::format::toString(data);
}
}  // namespace std
