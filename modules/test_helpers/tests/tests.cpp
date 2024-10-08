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





TYPED_TEST(TypeTests, OstreamTest) {
  const DummyType dummy_type;
  std::stringstream ss;
  EXPECT_TRUE(ss.str().empty());
  ss << dummy_type;
  EXPECT_FALSE(ss.str().empty());
}

TYPED_TEST(TypeTests, FmtFormatTest) {
  DummyType dummy_type;
  std::string fmt_stream;
  EXPECT_TRUE(fmt_stream.empty());
  fmt_stream = fmt::format("{}", dummy_type);
  EXPECT_FALSE(fmt_stream.empty());
}

TYPED_TEST(TypeTests, RandomUnequalTest) {
  auto [mt, mt_copy] = heph::random::createPairOfIdenticalRNGs();

  EXPECT_EQ(DummyType::random(mt), DummyType::random(mt_copy));  // Ensure determinism
  EXPECT_NE(DummyType::random(mt), DummyType::random(mt));       // Ensure randomness
}

}  // namespace heph::types::tests
