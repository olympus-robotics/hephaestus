//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <sstream>
#include <string>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/types/dummy_type.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::types::tests {

/* --- Test all custom structs which support creation via a random member function --- */
template <class T>
class TypeTests : public ::testing::Test {};
using TypeImplementations = ::testing::Types<DummyType>;

TYPED_TEST_SUITE(TypeTests, TypeImplementations);

TYPED_TEST(TypeTests, OstreamTest) {
  const TypeParam type;
  std::stringstream ss;
  EXPECT_TRUE(ss.str().empty());
  ss << type;
  EXPECT_FALSE(ss.str().empty());
}

TYPED_TEST(TypeTests, FmtFormatTest) {
  TypeParam type;
  std::string fmt_stream;
  EXPECT_TRUE(fmt_stream.empty());
  fmt_stream = fmt::format("{}", type);
  EXPECT_FALSE(fmt_stream.empty());
}

TYPED_TEST(TypeTests, RandomUnequalTest) {
  auto [mt, mt_copy] = heph::random::createPairOfIdenticalRNGs();

  EXPECT_EQ(TypeParam::random(mt), TypeParam::random(mt_copy));  // Ensure determinism
  EXPECT_NE(TypeParam::random(mt), TypeParam::random(mt));       // Ensure randomness
}

}  // namespace heph::types::tests
