//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include "hephaestus/utils/exception.h"

namespace heph::random {

//=================================================================================================
// Random boolean creation
//=================================================================================================
template <typename T>
concept IsBoolean = std::is_same_v<T, bool>;

template <IsBoolean T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::bernoulli_distribution dist;
  return dist(mt);
}

//=================================================================================================
// Random integer value creation
//=================================================================================================
template <typename T>
concept IsNonBooleanIntegral = std::integral<T> && !std::same_as<T, bool>;

template <IsNonBooleanIntegral T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::uniform_int_distribution<T> dist;
  return dist(mt);
}

//=================================================================================================
// Random floating point value creation
//=================================================================================================
template <std::floating_point T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::uniform_real_distribution<T> dist;
  return dist(mt);
}

//=================================================================================================
// Random enum creation
//=================================================================================================
template <typename T>
concept IsEnum = std::is_enum_v<T>;

template <IsEnum T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  static const auto enum_values = magic_enum::enum_values<T>();
  std::uniform_int_distribution<size_t> dist(0, enum_values.size() - 1);
  return enum_values[dist(mt)];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

//=================================================================================================
// Random timestamp creation
//=================================================================================================
template <typename T>
concept IsTimestamp = requires {
  typename T::clock;
  typename T::duration;
  requires std::is_same_v<typename T::clock, std::chrono::system_clock> ||
               std::is_same_v<typename T::clock, std::chrono::steady_clock>;
  requires std::is_same_v<T, typename std::chrono::time_point<typename T::clock, typename T::duration>>;
};

namespace internal {
template <IsTimestamp T, size_t Year>
[[nodiscard]] constexpr auto createFinalTimestampOfTheYear() -> T {
  // The final date of the year is YYYY-12-31.
  constexpr auto YEAR = std::chrono::year{ Year };
  constexpr auto FINAL_MONTH = std::chrono::December;
  constexpr auto FINAL_DAY = std::chrono::year_month_day_last{ YEAR / FINAL_MONTH / std::chrono::last };
  constexpr auto FINAL_DATE_SYS_DAYS = std::chrono::sys_days{ FINAL_DAY };

  // The final time of the day is 23:59:59.999...
  constexpr auto FINAL_TIME = std::chrono::hours{ 24 } - typename T::duration{ 1 };
  // The final timestamp of the year is YYYY-12-31 23:59:59.999...
  constexpr auto TOTAL_TIME = FINAL_DATE_SYS_DAYS + FINAL_TIME;

  return T{ TOTAL_TIME.time_since_epoch() };
}
}  // namespace internal

/// Create a random timestamp between year 1970 and the year 2100.
template <IsTimestamp T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  static constexpr auto MIN_DURATION = 0;  // Start of UNIX epoch time == year 1970.
  static constexpr auto FINAL_TIMESTAMP = internal::createFinalTimestampOfTheYear<T, 2100>();
  static constexpr auto MAX_DURATION = FINAL_TIMESTAMP.time_since_epoch().count();  // End of year 2100.

  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  const auto duration = typename T::duration{ duration_dist(mt) };
  const auto timestamp = T{ duration };

  return timestamp;
}

//=================================================================================================
// Random struct/class creation
//=================================================================================================
template <class T>
concept HasrandomMethod = requires(std::mt19937_64& mt) {
  { T::random(mt) } -> std::convertible_to<T>;
};

template <HasrandomMethod T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  return T::random(mt);
}

//=================================================================================================
// Concept for random creatable types
//=================================================================================================
template <class T>
concept IsRandomCreatable = requires(std::mt19937_64& mt) {
  { random<T>(mt) } -> std::convertible_to<T>;
};

//=================================================================================================
// Internal helper functions for container types
//=================================================================================================
namespace internal {
[[nodiscard]] inline auto getSize(std::mt19937_64& mt, std::optional<size_t> fixed_size,
                                  bool allow_empty) -> size_t {
  if (fixed_size.has_value()) {
    throwExceptionIf<InvalidParameterException>(
        allow_empty == false && fixed_size.value() == 0,
        fmt::format("fixed_size must be non-zero if allow_empty == true"));
    return fixed_size.value();
  }

  static constexpr auto MAX_SIZE = 42ul;
  const auto min_size = allow_empty ? 0ul : 1ul;
  std::uniform_int_distribution<size_t> dist(min_size, MAX_SIZE);

  return dist(mt);
}
}  // namespace internal

//=================================================================================================
// Random string creation
//=================================================================================================
template <typename T>
concept IsString = std::same_as<T, std::string>;

/// Generate a random string of characters, including special case characters and numbers.
template <IsString T>
[[nodiscard]] auto random(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                          bool allow_empty = true) -> T {
  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  static constexpr auto PRINTABLE_ASCII_START = 32;  // Space
  static constexpr auto PRINTABLE_ASCII_END = 126;   // Equivalency sign - tilde
  std::uniform_int_distribution<unsigned char> char_dist(PRINTABLE_ASCII_START, PRINTABLE_ASCII_END);

  std::string random_string;
  random_string.reserve(size);

  auto gen_random_char = [&mt, &char_dist]() { return static_cast<char>(char_dist(mt)); };
  std::generate_n(std::back_inserter(random_string), size, gen_random_char);

  return random_string;
}

//=================================================================================================
// Random vector creation
//=================================================================================================
namespace internal {
template <typename T>
concept IsVector =
    requires { typename T::value_type; } && (std::is_same_v<T, std::vector<typename T::value_type>>);
}  // namespace internal

template <typename T>
concept IsRandomCreatableVector = internal::IsVector<T> && IsRandomCreatable<typename T::value_type>;

/// Fill a vector with randomly generated values of type T.
template <IsRandomCreatableVector T>
[[nodiscard]] auto random(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                          bool allow_empty = true) -> T {
  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  T vec;
  vec.reserve(size);

  auto gen = [&mt]() -> typename T::value_type { return random<typename T::value_type>(mt); };
  std::generate_n(std::back_inserter(vec), size, gen);

  return vec;
};

}  // namespace heph::random
