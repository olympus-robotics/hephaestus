//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <future>
#include <memory>
#include <string>
#include <utility>

#include <fmt/base.h>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/influxdb_sink/influxdb_metric_sink.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

namespace telemetry_example {
namespace {
struct DummyMeasure {
  double error;
  int64_t counter;
  std::string message;
};
}  // namespace
}  // namespace telemetry_example

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;
  const heph::utils::StackTrace stack;
  heph::telemetry::makeAndRegisterLogSink<heph::telemetry::AbslLogSink>();

  try {
    static constexpr auto PERIOD = std::chrono::duration<double>(1.0);

    auto influxdb_sink = heph::telemetry_sink::InfluxDBSink::create({ .url = "localhost:8099",
                                                                      .token = "my-super-secret-auth-token",
                                                                      .database = "hephaestus",
                                                                      .flush_period = PERIOD });
    heph::telemetry::registerMetricSink(std::move(influxdb_sink));

    int64_t counter = 0;
    auto mt = heph::random::createRNG();
    auto spinner = heph::concurrency::Spinner(
        heph::concurrency::Spinner::createNeverStoppingCallback([&mt, &counter]() {
          heph::telemetry::record("telemetry_example", "dummy",
                                  telemetry_example::DummyMeasure{
                                      .error = heph::random::random<double>(mt),
                                      .counter = counter++,
                                      .message = heph::random::random<std::string>(mt, 4),
                                  });
        }),
        PERIOD);

    spinner.start();
    heph::utils::TerminationBlocker::waitForInterrupt();
    spinner.stop().get();

  } catch (std::exception& e) {
    fmt::println(stderr, "Execution terminated with exception: {}", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
