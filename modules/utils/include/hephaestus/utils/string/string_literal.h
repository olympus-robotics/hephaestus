//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace heph::utils::string {

/// Helper to convert a string literal to a std::string_view. This for example allows to use a literal string
/// "foo" as a non-type template parameter.
template <std::size_t Size>
struct StringLiteral {
  // Note: The following constructor is not marked explicit on purpose. It allows to use a string literal as a
  // non-type template parameter.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, google-explicit-constructor, hicpp-explicit-conversions)
  constexpr StringLiteral(const char (&str)[Size]);

  [[nodiscard]] constexpr explicit operator std::string_view() const noexcept;

  constexpr auto operator<=>(const StringLiteral&) const noexcept = default;

  std::array<char, Size> value;  // Does include the null terminator.
};

//=================================================================================================
// Implementation
//=================================================================================================

template <size_t Size>
constexpr StringLiteral<Size>::StringLiteral(
    const char (&str)[Size]) {  // NOLINT(cppcoreguidelines-avoid-c-arrays)
  std::copy_n(static_cast<const char*>(str), Size, value.begin());
}

template <size_t Size>
constexpr StringLiteral<Size>::StringLiteral::operator std::string_view() const noexcept {
  return { value.data(), Size - 1 };  // Remove the null terminator.
}

}  // namespace heph::utils::string
