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
  EXPECT_TRUE(isWithinBounds(-5, bounds));
  EXPECT_TRUE(isWithinBounds(0, bounds));
  EXPECT_TRUE(isWithinBounds(10, bounds));
  EXPECT_FALSE(isWithinBounds(-6, bounds));
  EXPECT_FALSE(isWithinBounds(11, bounds));
}

TEST(BoundsTest, LeftOpenBounds) {
  const Bounds<int> bounds{ .lower = -5, .upper = 10, .type = BoundsType::LEFT_OPEN };
  EXPECT_FALSE(isWithinBounds(-5, bounds));
  EXPECT_TRUE(isWithinBounds(0, bounds));
  EXPECT_TRUE(isWithinBounds(10, bounds));
  EXPECT_FALSE(isWithinBounds(-6, bounds));
  EXPECT_FALSE(isWithinBounds(11, bounds));
}

TEST(BoundsTest, RightOpenBounds) {
  const Bounds<int> bounds{ .lower = -5, .upper = 10, .type = BoundsType::RIGHT_OPEN };
  EXPECT_TRUE(isWithinBounds(-5, bounds));
  EXPECT_TRUE(isWithinBounds(0, bounds));
  EXPECT_FALSE(isWithinBounds(10, bounds));
  EXPECT_FALSE(isWithinBounds(-6, bounds));
  EXPECT_FALSE(isWithinBounds(11, bounds));
}

TEST(BoundsTest, OpenBounds) {
  const Bounds<int> bounds{ .lower = -5, .upper = 10, .type = BoundsType::OPEN };
  EXPECT_FALSE(isWithinBounds(-5, bounds));
  EXPECT_TRUE(isWithinBounds(0, bounds));
  EXPECT_FALSE(isWithinBounds(10, bounds));
  EXPECT_FALSE(isWithinBounds(-6, bounds));
  EXPECT_FALSE(isWithinBounds(11, bounds));
}

}  // namespace heph::types::tests
