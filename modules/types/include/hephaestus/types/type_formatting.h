//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <ranges>
#include <sstream>

#include <fmt/format.h>
#include <range/v3/view/zip.hpp>

namespace heph::types {

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
    ss << fmt::format("  Index: {}, Value: {}\n", index, value);
  }

  return ss.str();
}
};  // namespace heph::types
