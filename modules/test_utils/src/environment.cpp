// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================
#include "hephaestus/test_utils/environment.h"

namespace heph::test_utils {
namespace internal {
namespace {
DefaultEnvironment* default_test_environments = nullptr;  // NOLINT
}
void createDefaultTestEnvironment() {
  default_test_environments = dynamic_cast<heph::test_utils::internal::DefaultEnvironment*>(
      ::testing::AddGlobalTestEnvironment(new heph::test_utils::internal::DefaultEnvironment{}));  // NOLINT
}
auto defaultTestEnvironment() -> DefaultEnvironment* {
  return default_test_environments;
}
}  // namespace internal

auto mt() -> std::mt19937_64& {
  return internal::default_test_environments->mt();
}
}  // namespace heph::test_utils
