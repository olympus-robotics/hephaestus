// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================
#include "hephaestus/test_utils/environment.h"

#include <memory>
#include <random>

#include <gtest/gtest.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/utils/stack_trace.h"

namespace heph::test_utils {
namespace {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class MyEnvironment : public ::testing::Environment {
public:
  ~MyEnvironment() override = default;

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

// NOLINTBEGIN
static auto* const test_environments =
    dynamic_cast<MyEnvironment*>(::testing::AddGlobalTestEnvironment(new MyEnvironment{}));
// NOLINTEND
}  // namespace

auto mt() -> std::mt19937_64& {
  return test_environments->mt();
}

}  // namespace heph::test_utils
