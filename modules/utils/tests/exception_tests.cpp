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
  auto throwing_func = []() { throwException<TypeMismatchException>("type mismatch"); };
  EXPECT_THROW(throwing_func(), TypeMismatchException);

  try {
    throwing_func();
  } catch (std::exception& e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("modules/utils/tests/exception_tests.cpp:14] type mismatch"));
  }
#endif
}

}  // namespace heph::utils::tests
