//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <absl/synchronization/mutex.h>
#include <fmt/core.h>
#include <magic_enum.hpp>
#include <magic_enum_all.hpp>

#include "hephaestus/utils/exception.h"

namespace heph::containers {

namespace internal {
template <typename EnumT>
constexpr auto checkEnumValuesArePowerOf2() -> bool {
  constexpr auto POWER_OF_TWO = magic_enum::enum_for_each<EnumT>([](auto val) {
    constexpr EnumT VALUE = val;
    constexpr auto VALUE_T = static_cast<std::underlying_type_t<EnumT>>(VALUE);
    if constexpr (VALUE_T == 0) {
      return true;  // 0 is not power of 2
    }

    return (VALUE_T & static_cast<std::underlying_type_t<EnumT>>(VALUE_T - 1)) ==
           0;  // check if VALUE_T is power of 2
  });

  return std::all_of(POWER_OF_TWO.begin(), POWER_OF_TWO.end(), [](bool val) { return val; });
}

template <typename EnumT>
constexpr auto allEnumValuesMask() -> std::underlying_type_t<EnumT> {
  std::underlying_type_t<EnumT> mask{};
  magic_enum::enum_for_each<EnumT>(
      [&mask](auto enum_val) { mask |= magic_enum::enum_integer<EnumT>(enum_val); });
  return mask;
}

}  // namespace internal

template <typename EnumT>
concept UnsignedEnum =
    requires { std::is_enum_v<EnumT>&& std::is_unsigned_v<typename std::underlying_type_t<EnumT>>; };

/// This class allows to use enum classes as bit flags.
/// Enum classes need to satisfy three properties:
/// - Underline type is unsigned (checked by template concept)
/// - All values are power of 2 (checked by static assert)
/// - No duplicated values (not checked, and impossible to until we get reflection)
/// Usage:
/// enum class Flag : uint8_t { A = 1u << 1u, B = 1u << 2u, C = 1u << 3u };
/// BitFlag<Flag> flag{ Flag::A };
/// flag.set(Flag::B).set(Flag::C);
/// flag.has(Flag::B); // true
/// flag.unset(Flag::A);
/// Variable containing multiple flags can be created as constexpr:
/// constexpr auto D = BitFlag<Flag>{ Flag::B }.set(Flag::C); // == Flag::B | Flag::C
/// flag.hasAnyOf(D); // true
template <UnsignedEnum EnumT>
class BitFlag {
public:
  // see https://dietertack.medium.com/using-bit-flags-in-c-d39ec6e30f08
  using T = std::underlying_type_t<EnumT>;

  constexpr BitFlag() : value_{ 0 } {
  }

  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  constexpr BitFlag(EnumT value) : value_{ static_cast<T>(value) } {
    // NOTE: the constructor is not marked explicit to allow implicit conversion from EnumT to it.
    // This allows to use the member methods with EnumT values directly at the cost of allocating the BitFlag
    // value.
    // TODO: consider if there is a way to avoid this allocation.
    static_assert(internal::checkEnumValuesArePowerOf2<EnumT>(),
                  "Enum is not valid for BitFlag, its values must be power of 2.");
  }

  /// Constructs a BitFlag with the given underlying value.
  ///
  /// An 'InvalidParameterException' is thrown if there are bits set in 'underlying_value' which do not
  /// correspond to a valid enum value.
  constexpr explicit BitFlag(std::underlying_type_t<EnumT> underlying_value) : value_(underlying_value) {
    static_assert(internal::checkEnumValuesArePowerOf2<EnumT>(),
                  "Enum is not valid for BitFlag, its values must be power of 2.");

    throwExceptionIf<InvalidParameterException>(
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        (underlying_value & ~internal::allEnumValuesMask<EnumT>()) != 0,
        "Enum underlying value contains invalid bits (bits which don't correspond to a valid enum value).");
  }

  [[nodiscard]] auto operator==(const BitFlag&) const -> bool = default;

  constexpr auto reset() -> BitFlag& {
    value_ = 0;
    return *this;
  }

  /// Set the input flag(s)
  constexpr auto set(BitFlag flag) -> BitFlag& {
    value_ = value_ | flag.value_;
    return *this;
  }

  /// Unset the given flag(s)
  constexpr auto unset(BitFlag flag) -> BitFlag& {
    value_ = value_ & static_cast<T>(~flag.value_);
    return *this;
  }

  /// Returns true if the input flag is set.
  /// In case of multiple input flags, returns true if all input flags are set.
  /// `has` logically behaves as `hasAll`.
  [[nodiscard]] constexpr auto has(BitFlag flag) const -> bool {
    return (value_ & flag.value_) == flag.value_;
  }

  /// Returns true if any of the input flags are set.
  /// In case of a single input flag, hasAny is equivalent to has.
  [[nodiscard]] constexpr auto hasExactly(BitFlag flag) const -> bool {
    return (value_ & flag.value_) == value_;
  }

  /// Returns true if any of the input flags are set
  [[nodiscard]] constexpr auto hasAnyOf(BitFlag flag) const -> bool {
    return (value_ & flag.value_) != 0u;
  }

  /// Return the underlying type of the enum. This is to be used only for serialization.
  [[nodiscard]] constexpr auto getUnderlyingValue() const -> T {
    return value_;
  }

private:
  T value_;  // We need to store the underlying value to be able to store multiple flags which
             // do not reprenset any enum value.
};

}  // namespace heph::containers
