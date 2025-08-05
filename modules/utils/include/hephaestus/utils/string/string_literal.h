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
  constexpr explicit StringLiteral(std::array<char, Size> v);

  [[nodiscard]] constexpr explicit operator std::string_view() const noexcept;

  constexpr auto operator<=>(const StringLiteral&) const noexcept = default;

  std::array<char, Size> value;  // Does include the null terminator.
};

template <std::size_t SizeL, std::size_t SizeR>
[[nodiscard]] constexpr auto operator+(const StringLiteral<SizeL>& lhs, const StringLiteral<SizeR>& rhs)
    -> StringLiteral<SizeL + SizeR - 1>;

//=================================================================================================
// Implementation
//=================================================================================================

template <size_t Size>
constexpr StringLiteral<Size>::StringLiteral(
    const char (&str)[Size]) {  // NOLINT(cppcoreguidelines-avoid-c-arrays)
  std::copy_n(static_cast<const char*>(str), Size, value.begin());
}

template <size_t Size>
constexpr StringLiteral<Size>::StringLiteral(std::array<char, Size> v) : value(std::move(v)) {
}

template <size_t Size>
constexpr StringLiteral<Size>::StringLiteral::operator std::string_view() const noexcept {
  return { value.data(), Size - 1 };  // Remove the null terminator.
}

template <std::size_t SizeL, std::size_t SizeR>
[[nodiscard]] constexpr auto operator+(const StringLiteral<SizeL>& lhs, const StringLiteral<SizeR>& rhs)
    -> StringLiteral<SizeL + SizeR - 1> {
  std::array<char, SizeL + SizeR - 1> value{};
  std::copy_n(lhs.value.data(), SizeL - 1, value.data());
  std::copy_n(rhs.value.data(), SizeR, value.data() + SizeL - 1);

  return StringLiteral<SizeL + SizeR - 1>{ std::move(value) };
}

}  // namespace heph::utils::string
