//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <limits>
#include <random>
#include <string>

namespace heph::types {

/// @brief A class representing a UUID (Universally Unique Identifier) version 4, which is purely (pseudo)
/// randomly generated. We follow RFC 9562, which defines the UUIDv4 format. For details, see
/// https://www.rfc-editor.org/rfc/rfc9562.html.
/// To use a UuidV4 as a key in a hash map, we provide a custom hash function using the Abseil library's
/// `AbslHashValue`. Include <absl/hash/hash.h> in the implementation file and provide a custom hash function
/// to the container, e.g. std::unordered_map<heph::types::UuidV4, DataT, absl::Hash<heph::types::UuidV4>>.
struct UuidV4 {
  /// @brief Default comparison operator. We avoid using the spaceship operator (<=>) as random UUIDs (version
  /// 4) are not ordered.
  [[nodiscard]] auto operator==(const UuidV4&) const -> bool = default;

  /// @brief Generates a random valid UUIDv4 using the provided random number generator.
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> UuidV4;

  /// @brief Creates a UUIDv4 using an internal static random number generator.
  [[nodiscard]] static auto create() -> UuidV4;

  /// @brief Creates a UUIDv4 with all bits set to zero, 00000000-0000-0000-0000-000000000000.
  /// A Nil UUID value can be useful to communicate the absence of any other UUID value in situations that
  /// otherwise require or use a 128-bit UUID. A Nil UUID can express the concept "no such value here". Thus,
  /// it is reserved for such use as needed for implementation-specific situations.
  [[nodiscard]] static constexpr auto createNil() -> UuidV4;

  /// @brief Creates a UUIDv4 with all bits set to one, FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF.
  /// A Max UUID value can be used as a sentinel value in situations where a 128-bit UUID is required, but a
  /// concept such as "end of UUID list" needs to be expressed and is reserved for such use as needed for
  /// implementation-specific situations.
  [[nodiscard]] static constexpr auto createMax() -> UuidV4;

  [[nodiscard]] auto format() const -> std::string;

  /// @brief Function to allow the use of UuidV4 in hash-based containers via the Abseil library.
  template <class H>
  friend auto AbslHashValue(H h, const UuidV4& id) -> H;  // NOLINT(readability-identifier-naming)

  uint64_t high{ 0ULL };  //!< High 64 bits of the 128 bit UUID
  uint64_t low{ 0ULL };   //!< Low 64 bits of the 128 bit UUID
};

/* --- Implementation --- */

template <class H>
auto AbslHashValue(H h, const UuidV4& id) -> H {  // NOLINT(readability-identifier-naming)
  return H::combine(std::move(h), id.high, id.low);
}

constexpr auto UuidV4::createNil() -> UuidV4 {
  return { .high = 0, .low = 0 };
}

constexpr auto UuidV4::createMax() -> UuidV4 {
  return { .high = std::numeric_limits<uint64_t>::max(), .low = std::numeric_limits<uint64_t>::max() };
}

}  // namespace heph::types
