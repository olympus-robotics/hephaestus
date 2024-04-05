//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/utils/string_utils.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {

struct TestCase {
  std::string description;
  std::string str;
  std::string start_token;
  std::string end_token;
  bool include_end_token;
  std::string expected;
};

TEST(Truncate, Truncate) {
  const auto test_cases = std::array{ TestCase{ .description = "Truncate with include",
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

  for (const auto& test_case : test_cases) {
    const auto truncated =
        truncate(test_case.str, test_case.start_token, test_case.end_token, test_case.include_end_token);
    EXPECT_EQ(truncated, test_case.expected) << test_case.description;
  }
}
}  // namespace heph::utils::tests
