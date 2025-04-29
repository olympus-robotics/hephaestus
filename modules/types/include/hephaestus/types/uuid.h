//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

namespace heph::types {

/// @brief A class representing a UUID (Universally Unique Identifier).
struct Uuid {
  [[nodiscard]] auto operator==(const Uuid&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Uuid;

  [[nodiscard]] auto format() const -> std::string;

  uint64_t high;  //!< High 64 bits of the 128 bit UUID
  uint64_t low;   //!< Low 64 bits of the 128 bit UUID
};

}  // namespace heph::types
