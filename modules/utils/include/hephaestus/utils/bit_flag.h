//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <absl/synchronization/mutex.h>
#include <fmt/core.h>
#include <magic_enum_all.hpp>

namespace heph::utils {

template <typename EnumT>
concept UnsignedEnum =
    requires { std::is_enum_v<EnumT>&& std::is_unsigned_v<typename std::underlying_type<EnumT>::type>; };

template <UnsignedEnum EnumT>
constexpr auto checkEnumValuesArePowerOf2() -> bool {
  constexpr auto POWER_OF_TWO = magic_enum::enum_for_each<EnumT>([](auto val) {
    constexpr EnumT VALUE = val;
    constexpr auto VALUE_T = static_cast<std::underlying_type<EnumT>::type>(VALUE);
    return (VALUE_T & (VALUE_T - 1)) == 0;  // check if VALUE_T is power of 2
  });

  return std::all_of(POWER_OF_TWO.begin(), POWER_OF_TWO.end(), [](bool val) { return val; });
}

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
/// flag.hasAny(D); // true
template <UnsignedEnum EnumT>
class BitFlag {
public:
  // see https://dietertack.medium.com/using-bit-flags-in-c-d39ec6e30f08
  using T = std::underlying_type<EnumT>::type;

  constexpr explicit BitFlag(EnumT value) : value_{ value } {
    static_assert(checkEnumValuesArePowerOf2<EnumT>(),
                  "Enum is not valid for BitFlag, its values must be power of 2.");
  }

  /// Unset all flags and set the given flag.
  constexpr auto reset(EnumT flag) -> BitFlag& {
    value_ = flag;
    return *this;
  }

  /// Set the input flag
  constexpr auto set(EnumT flag) -> BitFlag& {
    value_ = static_cast<EnumT>(static_cast<T>(value_) | static_cast<T>(flag));
    return *this;
  }

  /// Unset the given flag
  constexpr auto unset(EnumT flag) -> BitFlag& {
    value_ =
        static_cast<EnumT>(static_cast<T>(value_) & ~static_cast<T>(flag));  // NOLINT(hicpp-signed-bitwise)
    return *this;
  }

  /// Retunrs true if the input flag is set
  [[nodiscard]] constexpr auto has(EnumT flag) const -> bool {
    return (static_cast<T>(value_) & static_cast<T>(flag)) == static_cast<T>(flag);
  }

  /// Retunrs true if the input flag is the only one set
  [[nodiscard]] constexpr auto hasOnly(EnumT flag) const -> bool {
    return (static_cast<T>(value_) & static_cast<T>(flag)) == static_cast<T>(value_);
  }

  /// Retunrs true if any of the input flags are set
  [[nodiscard]] constexpr auto hasAny(EnumT flag) const -> bool {
    return (static_cast<T>(value_) & static_cast<T>(flag)) != 0u;
  }

  constexpr operator EnumT() const {  // NOLINT(google-explicit-conversion, google-explicit-constructor,
                                      // hicpp-explicit-conversions)
    return value_;
  }

private:
  EnumT value_;
};

}  // namespace heph::utils
