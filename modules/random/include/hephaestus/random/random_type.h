//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <random>

#include <magic_enum.hpp>

namespace heph::random {

//=================================================================================================
// Random boolean generation
//=================================================================================================
template <typename T>
concept IsBooleanT = std::is_same_v<T, bool>;

template <IsBooleanT T>
[[nodiscard]] auto randomT(std::mt19937_64& mt) -> T {
  std::bernoulli_distribution dist;
  return dist(mt);
}

//=================================================================================================
// Random integer value generation
//=================================================================================================
template <typename T>
concept IsNonBooleanIntegralT = std::integral<T> && !std::same_as<T, bool>;

template <IsNonBooleanIntegralT T>
[[nodiscard]] auto randomT(std::mt19937_64& mt) -> T {
  std::uniform_int_distribution<T> dist;
  return dist(mt);
}

//=================================================================================================
// Random floating point value generation
//=================================================================================================
template <std::floating_point T>
[[nodiscard]] auto randomT(std::mt19937_64& mt) -> T {
  std::uniform_real_distribution<T> dist;
  return dist(mt);
}

//=================================================================================================
// Random enum generation
//=================================================================================================
template <typename T>
concept IsEnumT = std::is_enum_v<T>;

template <IsEnumT T>
[[nodiscard]] auto randomT(std::mt19937_64& mt) -> T {
  static const auto enum_values = magic_enum::enum_values<T>();
  std::uniform_int_distribution<size_t> dist(0, enum_values.size() - 1);
  return enum_values[dist(mt)];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

//=================================================================================================
// Random timestamp generation
//=================================================================================================
template <typename T>
concept IsTimestampT = requires {
  typename T::clock;
  requires std::is_same_v<typename T::clock, std::chrono::system_clock>;
};

namespace internal {
template <IsTimestampT T, size_t Year>
[[nodiscard]] constexpr auto createFinalTimestampOfTheYear() -> T {
  // THe final date of the year is YYYY-12-31.
  constexpr auto YEAR = std::chrono::year{ Year };
  constexpr auto FINAL_MONTH = std::chrono::December;
  constexpr auto FINAL_DAY = std::chrono::last;
  constexpr auto FINAL_YEAR_MONTH_DAY = std::chrono::year_month_day_last{ YEAR / FINAL_MONTH / FINAL_DAY };
  constexpr auto FINAL_DATE = T{ std::chrono::sys_days(FINAL_YEAR_MONTH_DAY) };

  // The final time of the day is 23:59:59.000...
  constexpr auto FINAL_TIME = std::chrono::duration_cast<typename T::duration>(std::chrono::hours{ 24 });

  // The final timestamp of the year is YYYY-12-31 23:59:59.000...
  return FINAL_DATE + FINAL_TIME;
}
}  // namespace internal

/// Generate a random timestamp between year 1970 and the year 2100.
template <IsTimestampT T>
[[nodiscard]] auto randomT(std::mt19937_64& mt) -> T {
  using DurationT = typename T::duration;

  static constexpr size_t MIN_DURATION = 0;  // Start of UNIX epoch time == year 1970.
  static constexpr auto FINAL_TIMESTAMP = internal::createFinalTimestampOfTheYear<T, 2100>();
  static constexpr size_t MAX_DURATION = FINAL_TIMESTAMP.time_since_epoch().count();  // End of year 2100.

  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  auto timestamp = T{ DurationT(duration_dist(mt)) };

  return timestamp;
}

//=================================================================================================
// Random struct/class generation
//=================================================================================================
template <class T>
concept HasCreateRandomMethodT = requires(std::mt19937_64& mt) {
  { T::random(mt) } -> std::convertible_to<T>;
};

template <HasCreateRandomMethodT T>
[[nodiscard]] auto randomT(std::mt19937_64& mt) -> T {
  return T::random(mt);
}

//=================================================================================================
// Concept for random generatable types
//=================================================================================================
template <class T>
concept IsRandomGeneratableT = requires(std::mt19937_64& mt) {
  { randomT<T>(mt) } -> std::convertible_to<T>;
};

}  // namespace heph::random
