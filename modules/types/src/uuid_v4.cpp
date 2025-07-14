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

namespace {
inline void setUuidVersion4(UuidV4& uuid) {
  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
  uuid.high &= 0xFFFFFFFFFFFF0FFFULL;  // Clear the version bits
  uuid.high |= 0x0000000000004000ULL;  // Set the version to 4 (random)
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

inline void setUuidVariantRFC9562(UuidV4& uuid) {
  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
  uuid.low &= 0x3FFFFFFFFFFFFFFFULL;  // Clear the variant bits
  uuid.low |= 0x8000000000000000ULL;  // Set the variant to RFC 9562 (10xx)
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}
}  // namespace

auto UuidV4::random(std::mt19937_64& mt) -> UuidV4 {
  static std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);
  auto uuid = UuidV4{ .high = dist(mt), .low = dist(mt) };

  setUuidVersion4(uuid);
  setUuidVariantRFC9562(uuid);

  return uuid;
}

auto UuidV4::create() -> UuidV4 {
  static auto rng = random::createRNG();
  static auto mt = std::mt19937_64(rng());

  return random(mt);
}

auto UuidV4::format() const -> std::string {
  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
  return fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
                     static_cast<uint32_t>(high >> 32ULL),                 // First 8 hex chars
                     static_cast<uint16_t>(high >> 16ULL),                 // Next 4 hex chars
                     static_cast<uint16_t>(high & 0xFFFFULL),              // Next 4 hex chars
                     static_cast<uint16_t>(low >> 48ULL),                  // Next 4 hex chars
                     static_cast<uint64_t>(low & 0x0000FFFFFFFFFFFFULL));  // Last 12 hex chars
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
}

}  // namespace heph::types
