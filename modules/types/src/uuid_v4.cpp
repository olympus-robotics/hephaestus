//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types/uuid_v4.h"

#include <cstdint>
#include <random>
#include <string>

#include <fmt/format.h>

#include "hephaestus/random/random_number_generator.h"

namespace heph::types {

auto UuidV4::random(std::mt19937_64& mt) -> UuidV4 {
  static std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
  auto uuid = UuidV4{ .high = dist(mt), .low = dist(mt) };

  // Set the version to 4 (random)
  uuid.high &= 0xFFFFFFFFFFFF0FFF;  // Clear the version bits
  uuid.high |= 0x0000000000004000;  // Set version to 4
  // Set the variant to RFC 9562 (10xx)
  uuid.low &= 0x3FFFFFFFFFFFFFFF;  // Clear the variant bits
  uuid.low |= 0x8000000000000000;  // Set the variant to RFC 9562

  return uuid;
}

auto UuidV4::create() -> UuidV4 {
  static auto rng = random::createRNG();
  static auto mt = std::mt19937_64(rng());

  return random(mt);
}

auto UuidV4::createNil() -> UuidV4 {
  return { .high = 0, .low = 0 };
}

auto UuidV4::createMax() -> UuidV4 {
  return { .high = UINT64_MAX, .low = UINT64_MAX };
}

auto UuidV4::format() const -> std::string {
  return fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
                     static_cast<uint32_t>(high >> 32),                // First 8 hex chars
                     static_cast<uint16_t>((high >> 16) & 0xFFFF),     // Next 4 hex chars
                     static_cast<uint16_t>(high & 0xFFFF),             // Next 4 hex chars
                     static_cast<uint16_t>(low >> 48),                 // Next 4 hex chars
                     static_cast<uint64_t>(low & 0xFFFFFFFFFFFFULL));  // Last 12 hex chars
}

}  // namespace heph::types
