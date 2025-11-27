//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types/uuid_v4.h"

#include <cstdint>
#include <ios>
#include <random>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include "hephaestus/error_handling/panic.h"
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

auto UuidV4::fromString(const std::string& uuid4_str) -> UuidV4 {
  static constexpr auto UUID4_STRING_SIZE = 36;
  // A UUID string must be 36 characters long
  HEPH_PANIC_IF(uuid4_str.length() != UUID4_STRING_SIZE, "Invalid UUID string length, expected {}, got {}",
                UUID4_STRING_SIZE, uuid4_str.length());

  std::stringstream ss(uuid4_str);
  char dash1{};
  char dash2{};
  char dash3{};
  char dash4{};

  // These parts map to the UUID format:
  // p1-p2-p3-p4-p5
  uint32_t p1{};  // 8 hex chars
  uint16_t p2{};  // 4 hex chars
  uint16_t p3{};  // 4 hex chars
  uint16_t p4{};  // 4 hex chars
  uint64_t p5{};  // 12 hex chars (fits in 48 bits)

  // Read all parts in hex format, consuming the dashes
  ss >> std::hex >> p1 >> dash1 >> p2 >> dash2 >> p3 >> dash3 >> p4 >> dash4 >> p5;

  // Check for any parsing errors (e.g., non-hex chars)
  // or if the dashes were not in the correct place.
  // We also check if there is any trailing data.
  const auto invalid = (ss.fail() || ss.rdbuf()->in_avail() != 0 || dash1 != '-' || dash2 != '-' ||
                        dash3 != '-' || dash4 != '-');
  HEPH_PANIC_IF(invalid, "Invalid UUID string format: {}", uuid4_str);

  UuidV4 uuid;

  // Assemble the 'high' part from p1, p2, and p3
  // (uint64_t)p1 << 32 | (uint64_t)p2 << 16 | (uint64_t)p3
  // [ p1: 32 bits ] [ p2: 16 bits ] [ p3: 16 bits ]
  uuid.high =
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, hicpp-signed-bitwise)
      (static_cast<uint64_t>(p1) << 32) | (static_cast<uint64_t>(p2) << 16) | static_cast<uint64_t>(p3);

  // Assemble the 'low' part from p4 and p5
  // (uint64_t)p4 << 48 | p5
  // [ p4: 16 bits ] [ p5: 48 bits ]
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, hicpp-signed-bitwise)
  uuid.low = ((static_cast<uint64_t>(p4) << 48) | p5);

  return uuid;
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
