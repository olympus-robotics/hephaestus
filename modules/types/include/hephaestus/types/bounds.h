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
  [[nodiscard]] constexpr auto operator==(const Bounds&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Bounds;

  T lower{};
  T upper{};
  BoundsType type{ BoundsType::INCLUSIVE };
};

template <NumericType T>
[[nodiscard]] static constexpr auto isWithinBounds(T value, const Bounds<T>& bounds) -> bool;

template <NumericType T>
[[nodiscard]] static constexpr auto clampValue(T value, const Bounds<T>& bounds) -> T;

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
constexpr auto isWithinBounds(T value, const Bounds<T>& bounds) -> bool {
  switch (bounds.type) {
    case BoundsType::INCLUSIVE:
      return value >= bounds.lower && value <= bounds.upper;
    case BoundsType::LEFT_OPEN:
      return value > bounds.lower && value <= bounds.upper;
    case BoundsType::RIGHT_OPEN:
      return value >= bounds.lower && value < bounds.upper;
    case BoundsType::OPEN:
      return value > bounds.lower && value < bounds.upper;
    default:
      throwException<InvalidParameterException>("Incorrect Type");
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable.
}

template <NumericType T>
constexpr auto clampValue(T value, const Bounds<T>& bounds) -> T {
  return std::clamp(value, bounds.lower, bounds.upper);
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
