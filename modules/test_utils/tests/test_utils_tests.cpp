//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/test_utils/heph_test.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::test_utils::tests {

TEST_F(HephTest, MtAccess) {
  std::ignore = mt();
}

}  // namespace heph::test_utils::tests
