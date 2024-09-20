//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <ranges>
#include <sstream>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <magic_enum.hpp>
#include <range/v3/view/zip.hpp>

#include "hephaestus/utils/concepts.h"

namespace heph::types {

//=================================================================================================
// Vector
//=================================================================================================
template <VectorType T>
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

//=================================================================================================
// UnorderedMap
//=================================================================================================
template <UnorderedMapType T>
[[nodiscard]] inline auto toString(const T& umap) -> std::string {
  std::stringstream ss;

  for (const auto& [key, value] : umap) {
    ss << "  Key: " << key << ", Value: " << value << '\n';
  }

  return ss.str();
}

//=================================================================================================
// Enum
//=================================================================================================
template <EnumType T>
[[nodiscard]] inline auto toString(T value) -> std::string_view {
  return magic_enum::enum_name(value);
}

};  // namespace heph::types
