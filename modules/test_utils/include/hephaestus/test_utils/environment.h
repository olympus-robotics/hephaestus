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
void createDefaultTestEnvironment();
}  // namespace internal

[[nodiscard]] auto mt() -> std::mt19937_64&;

}  // namespace heph::test_utils
