//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <ranges>
#include <sstream>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <range/v3/view/zip.hpp>

namespace heph::types {

//=================================================================================================
// Timepoint
//=================================================================================================
// A system clock provides access to time and date.
template <typename T>
concept IsSystemClock = std::is_same_v<T, std::chrono::system_clock>;

template <IsSystemClock T>
[[nodiscard]] inline auto toString(const std::chrono::time_point<T>& timestamp) -> std::string {
  return fmt::format("{:%Y-%m-%d %H:%M:%S}", timestamp);
}

// A steady clock does not have an anchor point in calendar time. It is only useful for measuring relative
// time intervals.
template <typename T>
concept IsSteadyClock = std::is_same_v<T, std::chrono::steady_clock>;

template <IsSteadyClock T>
[[nodiscard]] inline auto toString(const std::chrono::time_point<T>& timestamp) -> std::string {
  auto duration_since_epoch = timestamp.time_since_epoch();
  auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch).count();
  auto days = total_seconds / (24 * 3600);            // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  auto hours = (total_seconds % (24 * 3600)) / 3600;  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  auto minutes = (total_seconds % 3600) / 60;         // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  auto seconds = total_seconds % 60;                  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  auto sub_seconds = (duration_since_epoch - std::chrono::seconds(total_seconds)).count();

  static constexpr auto TIME_BASE = T::duration::period::den;  // sub-second precision
  static constexpr int64_t GIGA = 1'000'000'000;
  static constexpr int64_t SCALER = GIGA / TIME_BASE;  // 1 for time base nanoseconds, 1'000 for microseconds

  return fmt::format("{}d {:02}h:{:02}m:{:02}.{:09}s", days, hours, minutes, seconds, sub_seconds * SCALER);
}

//=================================================================================================
// Vector
//=================================================================================================
template <typename T>
concept IsVector = requires(T t) { std::is_same_v<T, std::vector<typename T::value_type>>; };

template <IsVector T>
[[nodiscard]] inline auto toString(const T& vec) -> std::string {
  std::stringstream ss;

  const auto indices = std::views::iota(0);
  const auto indexed_vec = ranges::views::zip(indices, vec);

  for (const auto& [index, value] : indexed_vec) {
    const std::string str = fmt::format("  Index: {}, Value: {}\n", index, value);
    ss << str;
  }

  return ss.str();
}
};  // namespace heph::types
