//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types/uuid.h"

#include <cstdint>
#include <string>

#include <fmt/format.h>

namespace heph::types {

auto Uuid::random(std::mt19937_64& mt) -> Uuid {
  return { .high = random::random<uint64_t>(mt), .low = random::random<uint64_t>(mt) };
}

auto Uuid::format() const -> std::string {
  return fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
                     static_cast<uint32_t>(high >> 32),                // First 8 hex chars
                     static_cast<uint16_t>((high >> 16) & 0xFFFF),     // Next 4 hex chars
                     static_cast<uint16_t>(high & 0xFFFF),             // Next 4 hex chars
                     static_cast<uint16_t>(low >> 48),                 // Next 4 hex chars
                     static_cast<uint64_t>(low & 0xFFFFFFFFFFFFULL));  // Last 12 hex chars
}

}  // namespace heph::types
