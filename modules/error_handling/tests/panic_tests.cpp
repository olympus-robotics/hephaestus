//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/error_handling/panic.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::error_handling::tests {
namespace {
constexpr int TEST_FORMAT_VALUE = 42;

TEST(Panic, Exception) {
  const error_handling::PanicAsExceptionScope panic_scope;
  EXPECT_THROW(panic("This is a panic with value {}", TEST_FORMAT_VALUE), PanicException);
}
}  // namespace
}  // namespace heph::error_handling::tests
