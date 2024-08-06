//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>

#include <fmt/format.h>
namespace heph::examples::types {

struct Accumulator {
  std::size_t value{ 0 };
  std::size_t counter{ 0 };
};
}  // namespace heph::examples::types

template <>
struct fmt::formatter<heph::examples::types::Accumulator> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const heph::examples::types::Accumulator& acc, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "value: {} | counter: {}", acc.value, acc.counter);
  }
};
