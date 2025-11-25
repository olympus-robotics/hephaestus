// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================
#include "hephaestus/test_utils/heph_test.h"

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/telemetry/metrics/metric_record.h"

namespace heph::test_utils {

HephTest::HephTest() : mt_{ heph::random::createRNG() } {
  telemetry::registerLogSink(std::make_unique<telemetry::AbslLogSink>(INFO));
}

HephTest::~HephTest() {
  telemetry::flushMetrics();
  telemetry::flushLogEntries();

  telemetry::removeAllLogSinks();
}

}  // namespace heph::test_utils
