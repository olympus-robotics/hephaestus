//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/format/format.h"

using namespace ::testing;  // NOLINT(google-build-using-namespace)

namespace heph::utils::format::tests {

//=================================================================================================
// Array
//=================================================================================================

TEST(TypeFormattingTests, ConvertEmptyArray) {
  const std::array<int, 0> arr = {};
  const std::string result = toString(arr);
  EXPECT_EQ(result, "");
}

TEST(TypeFormattingTests, ConvertIntArray) {
  const std::array<int, 3> arr = { 1, 2, 3 };
  const std::string result = toString(arr);
  const std::string expected = "  Index: 0, Value: 1\n"
                               "  Index: 1, Value: 2\n"
                               "  Index: 2, Value: 3\n";
  EXPECT_EQ(result, expected);
}

TEST(TypeFormattingTests, ConvertDoubleArray) {
  const std::array<double, 3> arr = { 1.1, 2.2, 3.3 };  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
  const std::string result = toString(arr);
  const std::string expected = "  Index: 0, Value: 1.1\n"
                               "  Index: 1, Value: 2.2\n"
                               "  Index: 2, Value: 3.3\n";
  EXPECT_EQ(result, expected);
}

#ifndef __GNUC__
TEST(TypeFormattingTests, ConvertStringArray) {
  const std::array<std::string, 3> arr = { "one", "two", "three" };
  const std::string result = toString(arr);
  const std::string expected = "  Index: 0, Value: one\n"
                               "  Index: 1, Value: two\n"
                               "  Index: 2, Value: three\n";
  EXPECT_EQ(result, expected);
}
#endif

//=================================================================================================
// Vector
//=================================================================================================

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

//=================================================================================================
// UnorderedMap
//=================================================================================================

TEST(UnorderedMapTests, ToStringEmpty) {
  const std::unordered_map<int, std::string> empty_umap;
  EXPECT_EQ(toString(empty_umap), "");
}

TEST(UnorderedMapTests, ToStringNonEmpty) {
  const std::unordered_map<int, std::string> umap{ { 1, "one" }, { 3, "three" }, { 2, "two" } };

  const std::vector<std::string> expected_outputs = {
    "  Key: 1, Value: one\n  Key: 2, Value: two\n  Key: 3, Value: three\n",
    "  Key: 1, Value: one\n  Key: 3, Value: three\n  Key: 2, Value: two\n",
    "  Key: 2, Value: two\n  Key: 1, Value: one\n  Key: 3, Value: three\n",
    "  Key: 2, Value: two\n  Key: 3, Value: three\n  Key: 1, Value: one\n",
    "  Key: 3, Value: three\n  Key: 1, Value: one\n  Key: 2, Value: two\n",
    "  Key: 3, Value: three\n  Key: 2, Value: two\n  Key: 1, Value: one\n"
  };

  const auto actual_output = toString(umap);

  int match_count = 0;
  for (const auto& expected_output : expected_outputs) {
    if (actual_output == expected_output) {
      ++match_count;
    }
  }

  EXPECT_EQ(match_count, 1);
}

//=================================================================================================
// Enum
//=================================================================================================

TEST(EnumTests, ToString) {
  enum class TestEnum : uint8_t { A, B, C };
  EXPECT_EQ(toString(TestEnum::A), "A");
  EXPECT_EQ(toString(TestEnum::B), "B");
  EXPECT_EQ(toString(TestEnum::C), "C");
}

//=================================================================================================
// ChronoTimePoint
//=================================================================================================

// Test assumes sub-second precision of at most nanoseconds.
TEST(TypeFormattingTests, ChronoTimestampFormattingSteadyClock) {
  const auto timestamp = std::chrono::steady_clock::now();
  const auto str = fmt::format("{}", toString(timestamp));

  ASSERT_LE(str.length(), 25);

  const auto it = std::ranges::find(str, 'd');
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
TEST(TypeFormattingTests, ChronoTimestampFormattingSystemClock) {
  const auto timestamp = std::chrono::system_clock::now();
  const auto str = fmt::format("{}", toString(timestamp));

  ASSERT_LE(str.size(), 26);

  ASSERT_EQ(str[0], '2');
  ASSERT_EQ(str[1], '0');
  ASSERT_EQ(str[4], '-');
  ASSERT_EQ(str[7], '-');
  ASSERT_EQ(str[10], ' ');
  ASSERT_EQ(str[13], ':');
  ASSERT_EQ(str[16], ':');
  ASSERT_EQ(str[19], '.');
}

}  // namespace heph::utils::format::tests
