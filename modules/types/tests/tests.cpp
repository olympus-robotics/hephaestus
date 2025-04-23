//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>

#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/types/bounds.h"
#include "hephaestus/types/dummy_type.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::types::tests {

using IntegerBoundsT = Bounds<int32_t>;
using FloatingPointBoundsT = Bounds<float>;

/* --- Test all custom structs which support creation via a random member function --- */
template <class T>
class TypeTests : public ::testing::Test {};
using TypeImplementations = ::testing::Types<IntegerBoundsT, FloatingPointBoundsT, DummyType>;

TYPED_TEST_SUITE(TypeTests, TypeImplementations);

TYPED_TEST(TypeTests, RandomUnequalTest) {
  auto [mt, mt_copy] = heph::random::createPairOfIdenticalRNGs();

  EXPECT_EQ(TypeParam::random(mt), TypeParam::random(mt_copy));  // Ensure determinism
  EXPECT_NE(TypeParam::random(mt), TypeParam::random(mt));       // Ensure randomness
}

}  // namespace heph::types::tests
