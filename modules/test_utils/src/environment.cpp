// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================
#include "hephaestus/test_utils/environment.h"

namespace heph::test_utils {
namespace internal {
namespace {
class DefaultEnvironment : public ::testing::Environment {
public:
  DefaultEnvironment() = default;
  DefaultEnvironment(const DefaultEnvironment&) = delete;
  DefaultEnvironment(DefaultEnvironment&&) = delete;
  auto operator=(const DefaultEnvironment&) -> DefaultEnvironment& = delete;
  auto operator=(DefaultEnvironment&&) -> DefaultEnvironment& = delete;

  ~DefaultEnvironment() override {
    telemetry::flushMetrics();
    telemetry::flushLogEntries();
  }

  void SetUp() override {
    telemetry::registerLogSink(std::make_unique<telemetry::AbslLogSink>(INFO));
  }

  [[nodiscard]] auto mt() -> std::mt19937_64& {
    return mt_;
  }

private:
  const utils::StackTrace trace_;
  std::mt19937_64 mt_{ heph::random::createRNG() };
  const error_handling::PanicAsExceptionScope panic_scope_;
};

DefaultEnvironment* default_test_environments = nullptr;  // NOLINT
}  // namespace

void createDefaultTestEnvironment() {
  default_test_environments = dynamic_cast<heph::test_utils::internal::DefaultEnvironment*>(
      ::testing::AddGlobalTestEnvironment(new heph::test_utils::internal::DefaultEnvironment{}));  // NOLINT
}

}  // namespace internal

auto mt() -> std::mt19937_64& {
  return internal::default_test_environments->mt();
}
}  // namespace heph::test_utils
