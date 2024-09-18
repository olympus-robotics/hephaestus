//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <concepts>
#include <cstdint>
#include <iostream>
#include <random>

#include <fmt/ostream.h>

#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/utils/exception.h"

namespace heph::types {

enum class BoundsType : uint8_t {
  INCLUSIVE,   // []
  LEFT_OPEN,   // (]
  RIGHT_OPEN,  // [)
  OPEN         // ()
};

template <typename T>
concept IsNumeric = std::integral<T> || std::floating_point<T>;

/// @brief A class representing range bounds.
template <IsNumeric T>
struct Bounds {
  [[nodiscard]] inline constexpr auto operator==(const Bounds&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Bounds;

  [[nodiscard]] inline constexpr auto isWithinBounds(T value) const -> bool;

  T lower_bound{};
  T upper_bound{};
  BoundsType bounds_type{ BoundsType::INCLUSIVE };
};

template <IsNumeric T>
auto operator<<(std::ostream& os, const Bounds<T>& bounds) -> std::ostream&;

//=================================================================================================
// IMPLEMENTATION
//=================================================================================================

template <IsNumeric T>
auto Bounds<T>::random(std::mt19937_64& mt) -> Bounds {
  return { .lower_bound = random::random<T>(mt),
           .upper_bound = random::random<T>(mt),
           .bounds_type = random::random<BoundsType>(mt) };
}

template <IsNumeric T>
inline constexpr auto Bounds<T>::isWithinBounds(T value) const -> bool {
  switch (bounds_type) {
    case BoundsType::INCLUSIVE:
      return value >= lower_bound && value <= upper_bound;
    case BoundsType::LEFT_OPEN:
      return value > lower_bound && value <= upper_bound;
    case BoundsType::RIGHT_OPEN:
      return value >= lower_bound && value < upper_bound;
    case BoundsType::OPEN:
      return value > lower_bound && value < upper_bound;
    default:
      throwException<InvalidParameterException>("Incorrect BoundsType");
  }
}

template <IsNumeric T>
auto operator<<(std::ostream& os, const Bounds<T>& bounds) -> std::ostream& {
  std::string bounds_type_str;
  switch (bounds.bounds_type) {
    case BoundsType::INCLUSIVE:
      bounds_type_str = "[]";
      break;
    case BoundsType::LEFT_OPEN:
      bounds_type_str = "(]";
      break;
    case BoundsType::RIGHT_OPEN:
      bounds_type_str = "[)";
      break;
    case BoundsType::OPEN:
      bounds_type_str = "()";
      break;
    default:
      throwException<InvalidParameterException>("Incorrect BoundsType");
  }

  return os << "Bounds: " << bounds_type_str[0] << bounds.lower_bound << " - " << bounds.upper_bound
            << bounds_type_str[1] << "\n";
}

}  // namespace heph::types

namespace fmt {
template <heph::types::IsNumeric T>
struct formatter<heph::types::Bounds<T>> : ostream_formatter {};
}  // namespace fmt
