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
#include "hephaestus/utils/concepts.h"
#include "hephaestus/utils/exception.h"

namespace heph::types {

enum class BoundsType : uint8_t {
  INCLUSIVE,   // []
  LEFT_OPEN,   // (]
  RIGHT_OPEN,  // [)
  OPEN         // ()
};

/// @brief A class representing range bounds.
template <NumericType T>
struct Bounds {
  [[nodiscard]] inline constexpr auto operator==(const Bounds&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Bounds;

  [[nodiscard]] inline constexpr auto isWithinBounds(T value) const -> bool;

  T lower{};
  T upper{};
  BoundsType type{ BoundsType::INCLUSIVE };
};

template <NumericType T>
auto operator<<(std::ostream& os, const Bounds<T>& bounds) -> std::ostream&;

//=================================================================================================
// IMPLEMENTATION
//=================================================================================================

template <NumericType T>
auto Bounds<T>::random(std::mt19937_64& mt) -> Bounds {
  return { .lower = random::random<T>(mt),
           .upper = random::random<T>(mt),
           .type = random::random<BoundsType>(mt) };
}

template <NumericType T>
inline constexpr auto Bounds<T>::isWithinBounds(T value) const -> bool {
  switch (type) {
    case BoundsType::INCLUSIVE:
      return value >= lower && value <= upper;
    case BoundsType::LEFT_OPEN:
      return value > lower && value <= upper;
    case BoundsType::RIGHT_OPEN:
      return value >= lower && value < upper;
    case BoundsType::OPEN:
      return value > lower && value < upper;
    default:
      throwException<InvalidParameterException>("Incorrect Type");
  }
}

template <NumericType T>
auto operator<<(std::ostream& os, const Bounds<T>& bounds) -> std::ostream& {
  std::string bounds_type_str;
  switch (bounds.type) {
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

  return os << "Bounds: " << bounds_type_str[0] << bounds.lower << " - " << bounds.upper << bounds_type_str[1]
            << "\n";
}

}  // namespace heph::types

namespace fmt {
template <heph::NumericType T>
struct formatter<heph::types::Bounds<T>> : ostream_formatter {};
}  // namespace fmt
