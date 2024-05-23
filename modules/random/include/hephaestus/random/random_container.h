//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include "hephaestus/random/random_type.h"
#include "hephaestus/utils/exception.h"

namespace heph::random {

namespace internal {
[[nodiscard]] inline auto getSize(std::mt19937_64& mt, std::optional<size_t> size,
                                  bool allow_empty) -> size_t {
  if (size.has_value()) {
    throwExceptionIf<InvalidParameterException>(allow_empty == false && size.value() == 0,
                                                fmt::format("Size must be non-zero if allow_empty == true"));
    return size.value();
  }

  static constexpr size_t MAX_SIZE = 42;
  std::uniform_int_distribution<size_t> dist(allow_empty == true ? 0 : 1, MAX_SIZE);
  return dist(mt);
}
}  // namespace internal

//=================================================================================================
// Random string generation
//=================================================================================================
template <typename T>
concept IsStringT = std::same_as<T, std::string>;

/// Generate a random string of characters, including special case characters and numbers.
template <IsStringT T>
[[nodiscard]] auto randomT(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
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
[[nodiscard]] auto randomT(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                           bool allow_empty = true) -> T {
  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  T vec;
  vec.reserve(size);

  auto gen = [&mt]() -> typename T::value_type { return randomT<typename T::value_type>(mt); };
  std::generate_n(std::back_inserter(vec), size, gen);

  return vec;
};

}  // namespace heph::random
