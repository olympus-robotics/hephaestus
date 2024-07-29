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

template <typename T>
concept IsClockT = requires(T t) {
  std::is_same_v<T, std::chrono::system_clock> || std::is_same_v<T, std::chrono::steady_clock> ||
      std::is_same_v<T, std::chrono::high_resolution_clock>;
};

template <IsClockT T>
[[nodiscard]] inline auto toString(const std::chrono::time_point<T>& timestamp) -> std::string {
  return fmt::format("{:%Y-%m-%d %H:%M:%S}", timestamp);
}

template <typename T>
concept IsVector = requires(T t) { std::is_same_v<T, std::vector<typename T::value_type>>; };

template <IsVector T>
[[nodiscard]] inline auto toString(const T& vec) -> std::string {
  std::stringstream ss;

  auto indices = std::views::iota(0);
  auto indexed_vec = ranges::views::zip(indices, vec);

  for (auto [index, value] : indexed_vec) {
    ss << "  Index: " << index << ", Value: " << value << '\n';
  }

  return ss.str();
}

};  // namespace heph::types
