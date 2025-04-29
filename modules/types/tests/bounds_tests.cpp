//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/types/bounds.h"

namespace heph::types::tests {

TEST(BoundsTest, InclusiveBounds) {
  const Bounds<float> bounds{ .lower = -5.0f, .upper = 10.0f, .type = BoundsType::INCLUSIVE };
  EXPECT_TRUE(isWithinBounds(-5.0f, bounds));
  EXPECT_TRUE(isWithinBounds(0.0f, bounds));
  EXPECT_TRUE(isWithinBounds(10.0f, bounds));
  EXPECT_FALSE(isWithinBounds(-6.0f, bounds));
  EXPECT_FALSE(isWithinBounds(11.0f, bounds));

  EXPECT_EQ(clampValue(-6.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-5.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-1.0f, bounds), -1.0f);
  EXPECT_EQ(clampValue(10.0f, bounds), 10.0f);
  EXPECT_EQ(clampValue(11.0f, bounds), 10.0f);
}

TEST(BoundsTest, LeftOpenBounds) {
  const Bounds<float> bounds{ .lower = -5.0f, .upper = 10.0f, .type = BoundsType::LEFT_OPEN };
  EXPECT_FALSE(isWithinBounds(-5.0f, bounds));
  EXPECT_TRUE(isWithinBounds(0.0f, bounds));
  EXPECT_TRUE(isWithinBounds(10.0f, bounds));
  EXPECT_FALSE(isWithinBounds(-6.0f, bounds));
  EXPECT_FALSE(isWithinBounds(11.0f, bounds));

  EXPECT_EQ(clampValue(-6.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-5.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-1.0f, bounds), -1.0f);
  EXPECT_EQ(clampValue(10.0f, bounds), 10.0f);
  EXPECT_EQ(clampValue(11.0f, bounds), 10.0f);
}

TEST(BoundsTest, RightOpenBounds) {
  const Bounds<float> bounds{ .lower = -5.0f, .upper = 10.0f, .type = BoundsType::RIGHT_OPEN };
  EXPECT_TRUE(isWithinBounds(-5.0f, bounds));
  EXPECT_TRUE(isWithinBounds(0.0f, bounds));
  EXPECT_FALSE(isWithinBounds(10.0f, bounds));
  EXPECT_FALSE(isWithinBounds(-6.0f, bounds));
  EXPECT_FALSE(isWithinBounds(11.0f, bounds));

  EXPECT_EQ(clampValue(-6.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-5.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-1.0f, bounds), -1.0f);
  EXPECT_EQ(clampValue(10.0f, bounds), 10.0f);
  EXPECT_EQ(clampValue(11.0f, bounds), 10.0f);
}

TEST(BoundsTest, OpenBounds) {
  const Bounds<float> bounds{ .lower = -5.0f, .upper = 10.0f, .type = BoundsType::OPEN };
  EXPECT_FALSE(isWithinBounds(-5.0f, bounds));
  EXPECT_TRUE(isWithinBounds(0.0f, bounds));
  EXPECT_FALSE(isWithinBounds(10.0f, bounds));
  EXPECT_FALSE(isWithinBounds(-6.0f, bounds));
  EXPECT_FALSE(isWithinBounds(11.0f, bounds));

  EXPECT_EQ(clampValue(-6.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-5.0f, bounds), -5.0f);
  EXPECT_EQ(clampValue(-1.0f, bounds), -1.0f);
  EXPECT_EQ(clampValue(10.0f, bounds), 10.0f);
  EXPECT_EQ(clampValue(11.0f, bounds), 10.0f);
}

}  // namespace heph::types::tests
