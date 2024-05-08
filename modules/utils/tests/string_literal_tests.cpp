//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <random>

#include <gtest/gtest.h>

#include "hephaestus/utils/string/string_literal.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::string::tests {

TEST(StringLiteralTests, ConstructorTest) {
  constexpr auto STRING_LITERAL = StringLiteral{ "foo" };
  EXPECT_EQ(STRING_LITERAL.value[0], 'f');
  EXPECT_EQ(STRING_LITERAL.value[1], 'o');
  EXPECT_EQ(STRING_LITERAL.value[2], 'o');
  EXPECT_EQ(STRING_LITERAL.value[3], '\0');
}

TEST(StringLiteralTests, OperatorStringViewTest) {
  static constexpr auto STRING_LITERAL = StringLiteral{ "foo" };
  constexpr auto STRING_VIEW = static_cast<std::string_view>(STRING_LITERAL);
  EXPECT_EQ(STRING_VIEW, "foo");
}

}  // namespace heph::utils::string::tests
