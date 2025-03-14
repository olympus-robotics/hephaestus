//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================
#pragma once

#include <string>
#include <type_traits>

#include <fmt/base.h>
#include <magic_enum.hpp>

template <typename T>
concept EnumType = std::is_enum_v<T>;

// Specialization of fmt::formatter for any enum type
template <EnumType E>
struct fmt::formatter<E> : fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(E value, FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", magic_enum::enum_name(value));
  }
};
