//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "eolo/utils/utils.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace eolo::utils::tests {

TEST(String, Truncate) {
  constexpr auto STR = "/path/to/some/file.txt";
  constexpr auto START_TOKEN = "to";
  constexpr auto END_TOKEN = ".txt";
  constexpr auto TRUNCATED = truncate(STR, START_TOKEN, END_TOKEN);
  constexpr auto EXPECTED = "to/some/file";
  EXPECT_EQ(TRUNCATED, EXPECTED);
}

TEST(String, TruncateInvalid) {
  constexpr auto STR = "/path/to/some/file.txt";
  constexpr auto START_TOKEN = "aaa";
  constexpr auto END_TOKEN = "bbb";
  constexpr auto TRUNCATED = truncate(STR, START_TOKEN, END_TOKEN);
  constexpr auto EXPECTED = STR;
  EXPECT_EQ(TRUNCATED, EXPECTED);
}

TEST(String, TruncateStartInvalid) {
  constexpr auto STR = "/path/to/some/file.txt";
  constexpr auto START_TOKEN = "aaa";
  constexpr auto END_TOKEN = ".txt";
  constexpr auto TRUNCATED = truncate(STR, START_TOKEN, END_TOKEN);
  constexpr auto EXPECTED = "/path/to/some/file";
  EXPECT_EQ(TRUNCATED, EXPECTED);
}

TEST(String, TruncateEndInvalid) {
  constexpr auto STR = "/path/to/some/file.txt";
  constexpr auto START_TOKEN = "some";
  constexpr auto END_TOKEN = "bbb";
  constexpr auto TRUNCATED = truncate(STR, START_TOKEN, END_TOKEN);
  constexpr auto EXPECTED = "some/file.txt";
  EXPECT_EQ(TRUNCATED, EXPECTED);
}

TEST(String, TruncateEmpty) {
  constexpr auto STR = "/path/to/some/file.txt";
  constexpr auto START_TOKEN = "";
  constexpr auto END_TOKEN = "";
  constexpr auto TRUNCATED = truncate(STR, START_TOKEN, END_TOKEN);
  constexpr auto EXPECTED = STR;
  EXPECT_EQ(TRUNCATED, EXPECTED);
}

}  // namespace eolo::utils::tests
