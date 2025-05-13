//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>

#include <gtest/gtest.h>

#include "hephaestus/containers/bit_flag.h"
#include "hephaestus/containers_proto/bit_flag.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/serdes/serdes.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::containers::tests {

TEST(BitFlagSerialization, RoundTrip) {
  enum class Enum : uint8_t { A = 1u << 3u, B = 1u << 5u };

  BitFlag<Enum> value;
  value.set(Enum::A);
  value.set(Enum::B);
  auto buff = serdes::serialize(value);

  BitFlag<Enum> value_des{};
  serdes::deserialize(buff, value_des);

  EXPECT_EQ(value, value_des);
}

}  // namespace heph::containers::tests
