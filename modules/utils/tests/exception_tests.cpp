//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {

static constexpr int TEST_FORMAT_VALUE = 42;
TEST(Exception, Throw) {
#ifndef DISABLE_EXCEPTIONS
  auto throwing_func = []() { panic("type mismatch {}", TEST_FORMAT_VALUE); };
  EXPECT_THROW_OR_DEATH(throwing_func(), Panic, "type mismatch 42");

  try {
    throwing_func();
  } catch (Panic& e) {
    EXPECT_THAT(e.what(),
                testing::MatchesRegex(fmt::format(
                    "^.*modules/utils/tests/exception_tests.cpp.*type mismatch {}.*$", TEST_FORMAT_VALUE)));
  }
#endif
}

TEST(Exception, ConditionalThrow) {
#ifndef DISABLE_EXCEPTIONS
  auto throwing_func = []() { panicIf(true, "type mismatch {}", TEST_FORMAT_VALUE); };
  EXPECT_THROW_OR_DEATH(throwing_func(), Panic, "type mismatch 42");

  try {
    throwing_func();
  } catch (Panic& e) {
    EXPECT_THAT(e.what(),
                testing::MatchesRegex(fmt::format(
                    "^.*modules/utils/tests/exception_tests.cpp.*type mismatch {}.*$", TEST_FORMAT_VALUE)));
  }
#endif
}

TEST(Exception, ConditionalNoThrow) {
#ifndef DISABLE_EXCEPTIONS
  auto throwing_func = []() { panicIf(false, "type mismatch", TEST_FORMAT_VALUE); };
  EXPECT_NO_THROW(throwing_func());
#endif
}

}  // namespace heph::utils::tests
