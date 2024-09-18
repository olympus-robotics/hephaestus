//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/types/bounds.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::types::tests {

TEST(BoundsTest, InclusiveBounds) {
  Bounds<int> bounds{ .lower_bound = -5, .upper_bound = 10, .bounds_type = BoundsType::INCLUSIVE };
  EXPECT_TRUE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_TRUE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

TEST(BoundsTest, LeftOpenBounds) {
  Bounds<int> bounds{ .lower_bound = -5, .upper_bound = 10, .bounds_type = BoundsType::LEFT_OPEN };
  EXPECT_FALSE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_TRUE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

TEST(BoundsTest, RightOpenBounds) {
  Bounds<int> bounds{ .lower_bound = -5, .upper_bound = 10, .bounds_type = BoundsType::RIGHT_OPEN };
  EXPECT_TRUE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_FALSE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

TEST(BoundsTest, OpenBounds) {
  Bounds<int> bounds{ .lower_bound = -5, .upper_bound = 10, .bounds_type = BoundsType::OPEN };
  EXPECT_FALSE(bounds.isWithinBounds(-5));
  EXPECT_TRUE(bounds.isWithinBounds(0));
  EXPECT_FALSE(bounds.isWithinBounds(10));
  EXPECT_FALSE(bounds.isWithinBounds(-6));
  EXPECT_FALSE(bounds.isWithinBounds(11));
}

}  // namespace heph::types::tests
