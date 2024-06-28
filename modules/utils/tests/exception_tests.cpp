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
  auto throwing_func = []() { throwException<TypeMismatchException>("type mismatch"); };
  EXPECT_THROW(throwing_func(), TypeMismatchException);

  try {
    throwing_func();
  } catch (std::exception& e) {
    EXPECT_THAT(e.what(), testing::HasSubstr("modules/utils/tests/exception_tests.cpp:16] type mismatch"));
  }
}

}  // namespace heph::utils::tests
