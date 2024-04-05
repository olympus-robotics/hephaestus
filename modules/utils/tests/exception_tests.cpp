//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
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
    EXPECT_STREQ(e.what(), "[modules/utils/tests/tests.cpp:13] type mismatch");
  }
}

}  // namespace heph::utils::tests
