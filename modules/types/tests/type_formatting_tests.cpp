//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include "hephaestus/types/type_formatting.h"

using namespace ::testing;  // NOLINT(google-build-using-namespace)

namespace heph::types::tests {

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
  std::unordered_map<int, std::string> empty_umap;
  EXPECT_EQ(toString(empty_umap), "");
}

TEST(UnorderedMapTests, ToStringNonEmpty) {
  std::unordered_map<int, std::string> umap{ { 1, "one" }, { 3, "three" }, { 2, "two" } };
  std::string expected_output = "  Key: 1, Value: one\n  Key: 3, Value: three\n  Key: 2, Value: two\n";
  EXPECT_EQ(toString(umap), expected_output);
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

}  // namespace heph::types::tests
