//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {
TEST(Exception, Throw) {
#ifndef DISABLE_EXCEPTIONS
  auto throwing_func = []() { panic("type mismatch {}", 42); };
  EXPECT_THROW_OR_DEATH(throwing_func(), Panic, "type mismatch");

  try {
    throwing_func();
  } catch (Panic& e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("modules/utils/tests/exception_tests.cpp:16] type mismatch"));
  }
#endif
}

TEST(Exception, ConditionalThrow) {
#ifndef DISABLE_EXCEPTIONS
  auto throwing_func = []() { panicIf(true, "type mismatch", 42); };
  EXPECT_THROW_OR_DEATH(throwing_func(), Panic, "type mismatch");

  try {
    throwing_func();
  } catch (Panic& e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("modules/utils/tests/exception_tests.cpp:16] type mismatch"));
  }
#endif
}

TEST(Exception, ConditionalNoThrow) {
#ifndef DISABLE_EXCEPTIONS
  auto throwing_func = []() { panicIf(false, "type mismatch", 42); };
  EXPECT_NO_THROW(throwing_func());
#endif
}

}  // namespace heph::utils::tests
