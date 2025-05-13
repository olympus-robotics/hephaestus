//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>
#include <limits>

#include <gtest/gtest.h>

#include "hephaestus/types/uuid_v4.h"

namespace heph::types::tests {

TEST(UuidV4Test, DefaultConstructor) {
  EXPECT_EQ(UuidV4(), UuidV4::createNil());
}

TEST(UuidV4Test, Create) {
  const auto uuid = UuidV4::create();

  // Expect randomness
  EXPECT_NE(uuid, UuidV4::create());

  // Expect version 4
  EXPECT_EQ((uuid.high & 0x000000000000F000ULL) >> 12ULL, 4ULL);

  // Expect variant RFC 9562
  EXPECT_EQ((uuid.low & 0xC000000000000000ULL) >> 62ULL, 2ULL);  // 10xx
}

TEST(UuidV4Test, CreateNil) {
  const auto uuid = UuidV4::createNil();

  // Expect all bits to be zero
  EXPECT_EQ(uuid.high, 0ULL);
  EXPECT_EQ(uuid.low, 0ULL);
}

TEST(UuidV4Test, CreateMax) {
  const auto uuid = UuidV4::createMax();

  // Expect all bits to be one
  EXPECT_EQ(uuid.high, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(uuid.low, std::numeric_limits<uint64_t>::max());
}

TEST(UuidV4Test, Format) {
  const auto uuid = UuidV4::create();

  // Check the format, we expect xxxxxxxx-xxxx-4xxx-8xxx-xxxxxxxxxxxx
  const auto formatted = uuid.format();
  EXPECT_EQ(formatted.size(), 36);
  EXPECT_EQ(formatted[8], '-');
  EXPECT_EQ(formatted[13], '-');
  EXPECT_EQ(formatted[14], '4');
  EXPECT_EQ(formatted[18], '-');
  EXPECT_TRUE(formatted[19] == '8' || formatted[19] == '9' || formatted[19] == 'a' ||
              formatted[19] == 'b');  // RFC 9562 variant bits are 10xx
  EXPECT_EQ(formatted[23], '-');
}

}  // namespace heph::types::tests
