//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/types/uuid_v4.h"

namespace heph::types::tests {

TEST(UuidV4Test, Create) {
  const auto uuid = UuidV4::create();

  // Expect randomness
  EXPECT_NE(uuid, UuidV4::create());

  // Expect version 4
  EXPECT_EQ((uuid.high & 0x000000000000F000) >> 12, 4);

  // Expect variant RFC 9562
  EXPECT_EQ((uuid.low & 0xC000000000000000) >> 62, 2);  // 10xx
}

TEST(UuidV4Test, CreateNil) {
  const auto uuid = UuidV4::createNil();

  // Expect all bits to be zero
  EXPECT_EQ(uuid.high, 0);
  EXPECT_EQ(uuid.low, 0);
}

TEST(UuidV4Test, CreateMax) {
  const auto uuid = UuidV4::createMax();

  // Expect all bits to be one
  EXPECT_EQ(uuid.high, UINT64_MAX);
  EXPECT_EQ(uuid.low, UINT64_MAX);
}

TEST(UuidV4Test, Format) {
  const auto uuid = UuidV4::create();

  // Check the format
  const auto formatted = uuid.format();
  EXPECT_EQ(formatted.size(), 36);
  EXPECT_EQ(formatted[8], '-');
  EXPECT_EQ(formatted[13], '-');
  EXPECT_EQ(formatted[18], '-');
  EXPECT_EQ(formatted[23], '-');
}

}  // namespace heph::types::tests
