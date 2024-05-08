//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <random>
#include <vector>

#include <magic_enum.hpp>

#include "hephaestus/testing/random_type.h"

namespace heph::testing {

//=================================================================================================
// Random vector generation
//=================================================================================================
namespace internal {
template <typename T>
concept IsVectorT =
    requires { typename T::value_type; } && (std::is_same_v<T, std::vector<typename T::value_type>>);
}  // namespace internal

template <typename T>
concept IsRandomGeneratableVectorT = internal::IsVectorT<T> && IsRandomGeneratableT<typename T::value_type>;

/// Fill a vector with randomly generated values of type T.
template <IsRandomGeneratableVectorT T>
[[nodiscard]] auto randomT(std::mt19937_64& mt, std::optional<size_t> size = std::nullopt) -> T {
  T vec;
  if (!size.has_value()) {
    static constexpr size_t MAX_SIZE = 42;
    std::uniform_int_distribution<size_t> dist(0, MAX_SIZE);
    size = dist(mt);
  }
  vec.reserve(size.value());

  auto gen = [&mt]() -> typename T::value_type { return randomT<typename T::value_type>(mt); };
  std::generate_n(std::back_inserter(vec), size.value(), gen);

  return vec;
};

}  // namespace heph::testing
