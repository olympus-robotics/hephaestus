// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================
#pragma once

#include <memory>
#include <random>

#include <gtest/gtest.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/utils/stack_trace.h"

namespace heph::test_utils {
namespace internal {
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

inline DefaultEnvironment* default_test_environments = nullptr;  // NOLINT

}  // namespace internal

[[nodiscard]] static inline auto mt() -> std::mt19937_64& {
  return internal::default_test_environments->mt();
}

}  // namespace heph::test_utils
