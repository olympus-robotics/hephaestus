//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <future>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include <fmt/core.h>
#include <nlohmann/detail/macro_scope.hpp>

#include "hephaestus/random/random_container.h"
#include "hephaestus/random/random_generator.h"
#include "hephaestus/random/random_type.h"
#include "hephaestus/telemetry/measure.h"
#include "hephaestus/telemetry/measure_sink.h"
#include "hephaestus/telemetry_sink/influxdb_measure_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

namespace telemetry_example {
constexpr auto MIN_DURATION = std::chrono::milliseconds{ 1000 }.count();
constexpr auto MAX_DURATION = std::chrono::milliseconds{ 5000 }.count();

struct DummyMeasure {
  double error;
  int64_t counter;
  std::string message;
};
// NOLINTNEXTLINE
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DummyMeasure, error, counter, message)

void run() {
  auto mt = heph::random::createRNG();

  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  while (!heph::utils::TerminationBlocker::stopRequested()) {
    auto now = heph::telemetry::ClockT::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(mt)));
    heph::telemetry::measure("telemetry_example", "motor1",
                             DummyMeasure{
                                 .error = heph::random::randomT<double>(mt),
                                 .counter = heph::random::randomT<int64_t>(mt),
                                 .message = heph::random::randomT<std::string>(mt, 4),
                             });
  }
}

}  // namespace telemetry_example

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;
  const heph::utils::StackTrace stack;

  try {
    // Register the sinks.
    {
      auto influxdb_sink = heph::telemetry_sink::InfluxDBSink::create(
          { .url = "localhost:8087", .token = "my-super-secret-auth-token", .database = "hephaestus" });
      heph::telemetry::registerMeasureSink(std::move(influxdb_sink));
    }

    // Navigation: demonstrates JSON serializable metric via nlohmann::json
    auto future = std::async(std::launch::async, &telemetry_example::run);

    future.get();
  } catch (std::exception& e) {
    fmt::println(stderr, "Execution terminated with exception: {}", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
