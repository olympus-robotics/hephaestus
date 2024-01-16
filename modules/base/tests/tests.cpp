//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================
#include <gtest/gtest.h>

#include "eolo/base/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace eolo::base::tests {

TEST(Exception, Throw) {
  auto throwing_func = []() { throwException<eolo::TypeMismatchException>("type mismatch"); };
  EXPECT_THROW(throwing_func(), eolo::TypeMismatchException);

  try {
    throwing_func();
  } catch (std::exception& e) {
    EXPECT_STREQ(e.what(), "[modules/base/tests/tests.cpp:14] type mismatch");
  }
}

}  // namespace eolo::base::tests
