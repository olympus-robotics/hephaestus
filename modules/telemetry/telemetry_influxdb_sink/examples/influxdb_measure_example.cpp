//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <future>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include <fmt/base.h>
#include <nlohmann/detail/macro_scope.hpp>

#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/telemetry_influxdb_sink/influxdb_metric_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

namespace telemetry_example {
namespace {
constexpr auto MIN_DURATION = std::chrono::milliseconds{ 1000 }.count();
constexpr auto MAX_DURATION = std::chrono::milliseconds{ 5000 }.count();

struct DummyMeasure {
  double error;
  int64_t counter;
  std::string message;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(DummyMeasure, error, counter, message)

void run() {
  auto mt = heph::random::createRNG();

  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  for (std::size_t counter = 0; !heph::utils::TerminationBlocker::stopRequested(); ++counter) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(mt)));
    heph::telemetry::record("telemetry_example", "dummy",
                            DummyMeasure{
                                .error = heph::random::random<double>(mt),
                                .counter = static_cast<int64_t>(counter),
                                .message = heph::random::random<std::string>(mt, 4),
                            });
  }
}
}  // namespace
}  // namespace telemetry_example

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;
  const heph::utils::StackTrace stack;

  try {
    auto influxdb_sink = heph::telemetry_sink::InfluxDBSink::create({ .url = "localhost:8099",
                                                                      .token = "my-super-secret-auth-token",
                                                                      .database = "hephaestus",
                                                                      .flush_rate_hz = 10 });
    heph::telemetry::registerMetricSink(std::move(influxdb_sink));

    auto spinner = heph::concurrency::Spinner(
        heph::concurrency::Spinner::createNeverStoppingCallback([]() { telemetry_example::run(); }), 1);

    spinner.start();
    heph::utils::TerminationBlocker::waitForInterrupt();
    spinner.stop().get();

  } catch (std::exception& e) {
    fmt::println(stderr, "Execution terminated with exception: {}", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
