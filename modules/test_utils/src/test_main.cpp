// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================

#include <gtest/gtest.h>

#include "hephaestus/test_utils/environment.h"

auto main(int argc, char** argv) -> int {
  ::testing::InitGoogleTest(&argc, argv);

  heph::test_utils::internal::default_test_environments =
      dynamic_cast<heph::test_utils::internal::DefaultEnvironment*>(::testing::AddGlobalTestEnvironment(
          new heph::test_utils::internal::DefaultEnvironment{}));  // NOLINT

  return RUN_ALL_TESTS();
}
