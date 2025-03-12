//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <exception>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/utils/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {
TEST(Exception, Throw) {
#ifndef DISABLE_EXCEPTIONS
  auto throwing_func = []() { panic("type mismatch"); };
  EXPECT_THROW_OR_DEATH(throwing_func(), Panic, "type mismatch");

  try {
    throwing_func();
  } catch (Panic& e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("modules/utils/tests/exception_tests.cpp:17] type mismatch"));
  }
#endif
}

}  // namespace heph::utils::tests
