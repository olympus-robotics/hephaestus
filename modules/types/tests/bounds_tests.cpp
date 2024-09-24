//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/types/bounds.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::types::tests {

TEST(BoundsTest, InclusiveBounds) {
  const Bounds<int> bounds{ .lower = -5, .upper = 10, .type = BoundsType::INCLUSIVE };
  EXPECT_TRUE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_TRUE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

TEST(BoundsTest, LeftOpenBounds) {
  const Bounds<int> bounds{ .lower = -5, .upper = 10, .type = BoundsType::LEFT_OPEN };
  EXPECT_FALSE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_TRUE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

TEST(BoundsTest, RightOpenBounds) {
  const Bounds<int> bounds{ .lower = -5, .upper = 10, .type = BoundsType::RIGHT_OPEN };
  EXPECT_TRUE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_FALSE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

TEST(BoundsTest, OpenBounds) {
  const Bounds<int> bounds{ .lower = -5, .upper = 10, .type = BoundsType::OPEN };
  EXPECT_FALSE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_FALSE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

}  // namespace heph::types::tests
