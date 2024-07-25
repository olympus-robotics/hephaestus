//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/string/string_utils.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::string::tests {

struct TestCase {
  std::string_view description;
  std::string_view str;
  std::string_view start_token;
  std::string_view end_token;
  bool include_end_token;
  std::string_view expected;
};

TEST(StringUtilsTests, Truncate) {
  constexpr auto TEST_CASES = std::array{ TestCase{ .description = "Truncate with include",
                                                    .str = "/path/to/some/file.txt",
                                                    .start_token = "to",
                                                    .end_token = ".txt",
                                                    .include_end_token = true,
                                                    .expected = "to/some/file.txt" },
                                          TestCase{ .description = "Truncate with exclude",
                                                    .str = "/path/to/some/file.txt",
                                                    .start_token = "to",
                                                    .end_token = ".txt",
                                                    .include_end_token = false,
                                                    .expected = "to/some/file" },
                                          TestCase{ .description = "Truncate invalid tokens",
                                                    .str = "/path/to/some/file.txt",
                                                    .start_token = "aaa",
                                                    .end_token = "bbb",
                                                    .include_end_token = true,
                                                    .expected = "/path/to/some/file.txt" },
                                          TestCase{ .description = "Truncate start invalid",
                                                    .str = "/path/to/some/file.txt",
                                                    .start_token = "aaa",
                                                    .end_token = ".txt",
                                                    .include_end_token = false,
                                                    .expected = "/path/to/some/file" },
                                          TestCase{ .description = "Truncate end invalid",
                                                    .str = "/path/to/some/file.txt",
                                                    .start_token = "some",
                                                    .end_token = "bbb",
                                                    .include_end_token = true,
                                                    .expected = "some/file.txt" },
                                          TestCase{ .description = "Truncate start and end empty",
                                                    .str = "/path/to/some/file.txt",
                                                    .start_token = "",
                                                    .end_token = "",
                                                    .include_end_token = true,
                                                    .expected = "/path/to/some/file.txt" } };

  for (const auto& test_case : TEST_CASES) {
    const auto truncated =
        truncate(test_case.str, test_case.start_token, test_case.end_token, test_case.include_end_token);
    EXPECT_EQ(truncated, test_case.expected) << test_case.description;
  }

  constexpr auto CONSTEXPR_TEST_CASE = TEST_CASES[0];
  constexpr auto CONSTEXPR_TRUNCATED =
      truncate(CONSTEXPR_TEST_CASE.str, CONSTEXPR_TEST_CASE.start_token, CONSTEXPR_TEST_CASE.end_token,
               CONSTEXPR_TEST_CASE.include_end_token);
  static_assert(CONSTEXPR_TRUNCATED == CONSTEXPR_TEST_CASE.expected);
}

TEST(StringUtilsTests, toUpperCase) {
  const std::string test_string = "aNy_TEST_CaSe_42!";
  const auto upper_case = toUpperCase(test_string);

  EXPECT_EQ(upper_case, "ANY_TEST_CASE_42!");
}

TEST(StringUtilsTests, toSnakeCase) {
  const std::string camel_case = "snakeCaseTest42!";
  const auto snake_case = toSnakeCase(camel_case);

  EXPECT_EQ(snake_case, "snake_case_test42!");
}

TEST(StringUtilsTests, toScreamingSnakeCase) {
  const std::string camel_case = "screamingSnakeCaseTest42!";
  const auto screaming_snake_case = toScreamingSnakeCase(camel_case);

  EXPECT_EQ(screaming_snake_case, "SCREAMING_SNAKE_CASE_TEST42!");
}

TEST(StringUtilsTests, stringToInt64) {
  const std::string int_str = "42";
  auto int_value = stringToInt64(int_str);
  EXPECT_THAT(int_value, Optional(42));

  const std::string negative_int_str = "-42";
  int_value = stringToInt64(negative_int_str);
  EXPECT_THAT(int_value, Optional(-42));

  const std::string invalid_str = "42a";
  auto invalid_value = stringToInt64(invalid_str);
  EXPECT_THAT(invalid_value, Eq(std::nullopt));

  const std::string double_str = "42.0";
  invalid_value = stringToInt64(double_str);
  EXPECT_THAT(invalid_value, Eq(std::nullopt));

  const std::string empty_str;
  invalid_value = stringToInt64(empty_str);
  EXPECT_THAT(invalid_value, Eq(std::nullopt));
}

}  // namespace heph::utils::string::tests
