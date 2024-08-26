//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/types/type_formatting.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::types::tests {

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
