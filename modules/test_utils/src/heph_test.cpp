// =====================================================================================
// |   This file is part of the FORKIFY project by FILICS GmbH. All rights reserved.   |
// =====================================================================================
#include "hephaestus/test_utils/heph_test.h"

#include <cassert>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/telemetry/metrics/metric_record.h"

namespace heph::test_utils {

HephTest::HephTest()
  : mt{ heph::random::createRNG() }
  , sink_ref_{ telemetry::makeAndRegisterLogSink<telemetry::AbslLogSink>(DEBUG) } {
}

HephTest::~HephTest() {
  telemetry::flushMetrics();
  telemetry::flushLogEntries();

  [[maybe_unused]] const bool sink_removal_successful = telemetry::removeLogSink(sink_ref_);
  assert(sink_removal_successful);
}

}  // namespace heph::test_utils
