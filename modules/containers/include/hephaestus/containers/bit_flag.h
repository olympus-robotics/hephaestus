//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <random>
#include <type_traits>

#include <magic_enum.hpp>
#include <magic_enum_utility.hpp>

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
    requires { std::is_enum_v<EnumT> && std::is_unsigned_v<typename std::underlying_type_t<EnumT>>; };

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
///
/// Note: This class uses magic_enum, which by default only supports enum values in the range -127..128. If
/// your enum contains larger values, then 'magic_enum::customize::enum_range<EnumT>' needs to be implemented
/// for that type. See https://github.com/Neargye/magic_enum/blob/master/doc/limitations.md#enum-range for
/// more details.
template <UnsignedEnum Enum>
class BitFlag {
public:
  // see https://dietertack.medium.com/using-bit-flags-in-c-d39ec6e30f08
  using EnumT = Enum;
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
  /// An 'Panic' is thrown if there are bits set in 'underlying_value' which do not
  /// correspond to a valid enum value.
  constexpr explicit BitFlag(T underlying_value) : value_(underlying_value) {
    static_assert(internal::checkEnumValuesArePowerOf2<EnumT>(),
                  "Enum is not valid for BitFlag, its values must be power of 2.");
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    panicIf(
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

  // Set the input flag(s) to `value`.
  constexpr auto set(BitFlag flag, bool value) -> BitFlag& {
    if (value) {
      set(flag);
    } else {
      unset(flag);
    }
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

template <typename T>
concept IsBitFlag = requires {
  typename T::EnumT;
  requires std::same_as<T, heph::containers::BitFlag<typename T::EnumT>>;
};

}  // namespace heph::containers

namespace heph::random {
template <containers::UnsignedEnum EnumT>
[[nodiscard]] auto random(std::mt19937_64& mt) -> containers::BitFlag<EnumT> {
  containers::BitFlag<EnumT> bit_flag{};

  auto enum_value_count = magic_enum::enum_values<EnumT>().size();
  auto values = random<std::size_t>(mt, { 0, enum_value_count - 1 });
  for (std::size_t i = 0; i < values; ++i) {
    const auto enum_v = random<EnumT>(mt);
    bit_flag.set(enum_v);
  }

  return bit_flag;
}
}  // namespace heph::random
