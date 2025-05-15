//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>

namespace heph::conduit {
enum struct RetrievalMethod : std::uint8_t {
  BLOCK,
  POLL,
};

template <std::size_t DepthV = 1, RetrievalMethod RetrievalMethodV = RetrievalMethod::BLOCK>
struct InputPolicy {
  static constexpr auto DEPTH = DepthV;
  static constexpr auto RETRIEVAL_METHOD = RetrievalMethodV;
  static_assert(DepthV > 0, "0 Depth does not make sense");
};

enum struct InputState : std::uint8_t {
  OK,
  OVERFLOW,
};
}  // namespace heph::conduit
