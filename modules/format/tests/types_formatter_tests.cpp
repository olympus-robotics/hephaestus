

//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdint>
#include <sstream>
#include <string>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "hephaestus/format/generic_formatter.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/types/bounds.h"
#include "hephaestus/types/dummy_type.h"

namespace heph::types::tests {

template <class T>
class TypeFormatTests : public ::testing::Test {};

using IntegerBoundsT = Bounds<int32_t>;
using FloatingPointBoundsT = Bounds<float>;
using TypeImplementations = ::testing::Types<IntegerBoundsT, FloatingPointBoundsT, DummyType>;

TYPED_TEST_SUITE(TypeFormatTests, TypeImplementations);

TYPED_TEST(TypeFormatTests, OstreamTest) {
  const TypeParam type{};
  std::stringstream ss;
  EXPECT_TRUE(ss.str().empty());
  ss << type;
  EXPECT_FALSE(ss.str().empty());
}

TYPED_TEST(TypeFormatTests, FmtFormatTest) {
  TypeParam type;
  std::string fmt_stream;
  EXPECT_TRUE(fmt_stream.empty());
  fmt_stream = fmt::format("{}", type);
  EXPECT_FALSE(fmt_stream.empty());
}

}  // namespace heph::types::tests
