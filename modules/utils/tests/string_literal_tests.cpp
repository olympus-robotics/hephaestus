//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <random>

#include <gtest/gtest.h>

#include "hephaestus/utils/string/string_literal.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::string::tests {

/// Generate a random string of characters, including special case characters and numbers.
auto randomString(std::mt19937_64& mt) -> std::string {
  static constexpr auto PRINTABLE_ASCII_START = 32;  // Space
  static constexpr auto PRINTABLE_ASCII_END = 126;   // Equivalency sign - tilde
  static constexpr size_t MAX_LENGTH = 127;

  std::uniform_int_distribution<unsigned char> char_dist(PRINTABLE_ASCII_START, PRINTABLE_ASCII_END);
  std::uniform_int_distribution<size_t> size_dist(0, MAX_LENGTH);

  auto size = size_dist(mt);
  std::string random_string;
  random_string.reserve(size);

  auto gen_random_char = [&mt, &char_dist]() { return static_cast<char>(char_dist(mt)); };
  std::generate_n(std::back_inserter(random_string), size, gen_random_char);

  return random_string;
}

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
