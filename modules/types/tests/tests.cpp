//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>

#include <absl/hash/hash_testing.h>
#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/types/bounds.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types/uuid_v4.h"

namespace heph::types::tests {

using IntegerBoundsT = Bounds<int32_t>;
using FloatingPointBoundsT = Bounds<float>;

//=================================================================================================
// Test all custom structs which support creation via a random member function
//=================================================================================================
template <class T>
class TypeTests : public ::testing::Test {};
using TypeImplementations = ::testing::Types<IntegerBoundsT, FloatingPointBoundsT, DummyType, UuidV4>;

TYPED_TEST_SUITE(TypeTests, TypeImplementations);

TYPED_TEST(TypeTests, RandomUnequalTest) {
  auto [mt, mt_copy] = random::createPairOfIdenticalRNGs();

  EXPECT_EQ(TypeParam::random(mt), TypeParam::random(mt_copy));  // Ensure determinism
  EXPECT_NE(TypeParam::random(mt), TypeParam::random(mt));       // Ensure randomness
}

//=================================================================================================
// Test types which support hashing
//=================================================================================================
template <class T>
class HashingTest : public ::testing::Test {};
using HashingImplementations = ::testing::Types<UuidV4>;

TYPED_TEST_SUITE(HashingTest, HashingImplementations);

TYPED_TEST(HashingTest, TestHash) {
  auto mt = random::createRNG();

  EXPECT_TRUE(absl::VerifyTypeImplementsAbslHashCorrectly({
      TypeParam(),
      TypeParam::random(mt),
  }));
}

}  // namespace heph::types::tests
