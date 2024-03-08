//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <gtest/gtest.h>

#include "hephaestus/base/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::base::tests {
TEST(Exception, Throw) {
  auto throwing_func = []() { throwException<heph::TypeMismatchException>("type mismatch"); };
  EXPECT_THROW(throwing_func(), heph::TypeMismatchException);

  try {
    throwing_func();
  } catch (std::exception& e) {
    EXPECT_STREQ(e.what(), "[modules/base/tests/tests.cpp:13] type mismatch");
  }
}

}  // namespace heph::base::tests
