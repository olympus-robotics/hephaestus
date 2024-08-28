//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "hephaestus/types/type_formatting.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::types::tests {

// Test assumes sub-second precision of at most nanoseconds.
TEST(TypeFormattingTests, TimestampFormattingSteadyClock) {
  const auto timestamp = std::chrono::steady_clock::now();
  const auto str = fmt::format("{}", toString(timestamp));

  ASSERT_LE(str.length(), 24);

  const auto it = std::find(str.begin(), str.end(), 'd');
  ASSERT_TRUE(it != str.end());
  ASSERT_EQ(*(it + 1), ' ');
  ASSERT_EQ(*(it + 4), 'h');
  ASSERT_EQ(*(it + 5), ':');
  ASSERT_EQ(*(it + 8), 'm');
  ASSERT_EQ(*(it + 9), ':');
  ASSERT_EQ(*(it + 12), '.');
  ASSERT_EQ(str.back(), 's');
}

// Test assumes sub-second precision of at most nanoseconds.
TEST(TypeFormattingTests, TimestampFormattingSystemClock) {
  const auto timestamp = std::chrono::system_clock::now();
  const auto str = fmt::format("{}", toString(timestamp));

  ASSERT_TRUE(str.length() <= 27);

  ASSERT_EQ(str[0], '2');
  ASSERT_EQ(str[1], '0');
  ASSERT_EQ(str[4], '-');
  ASSERT_EQ(str[7], '-');
  ASSERT_EQ(str[10], ' ');
  ASSERT_EQ(str[13], ':');
  ASSERT_EQ(str[16], ':');
  ASSERT_EQ(str[19], '.');
}

TEST(TypeFormattingTests, ConvertEmptyVector) {
  const std::vector<int> vec;
  const std::string result = toString(vec);
  EXPECT_EQ(result, "");
}

TEST(TypeFormattingTests, ConvertIntVector) {
  const std::vector<int> vec = { 1, 2, 3 };
  const std::string result = toString(vec);
  const std::string expected = "  Index: 0, Value: 1\n"
                               "  Index: 1, Value: 2\n"
                               "  Index: 2, Value: 3\n";
  EXPECT_EQ(result, expected);
}

TEST(TypeFormattingTests, ConvertDoubleVector) {
  const std::vector<double> vec = { 1.1, 2.2, 3.3 };  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  const std::string result = toString(vec);
  const std::string expected = "  Index: 0, Value: 1.1\n"
                               "  Index: 1, Value: 2.2\n"
                               "  Index: 2, Value: 3.3\n";
  EXPECT_EQ(result, expected);
}

TEST(TypeFormattingTests, ConvertStringVector) {
  const std::vector<std::string> vec = { "one", "two", "three" };
  const std::string result = toString(vec);
  const std::string expected = "  Index: 0, Value: one\n"
                               "  Index: 1, Value: two\n"
                               "  Index: 2, Value: three\n";
  EXPECT_EQ(result, expected);
}

}  // namespace heph::types::tests
