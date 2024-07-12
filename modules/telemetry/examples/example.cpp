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
// #include <string_view>
#include <thread>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "example_proto_conversion.h"
#include "hephaestus/random/random_container.h"
#include "hephaestus/random/random_generator.h"
#include "hephaestus/random/random_type.h"
#include "hephaestus/telemetry/sink.h"
#include "hephaestus/telemetry/telemetry.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

namespace telemetry_example {
constexpr auto MIN_DURATION = std::chrono::milliseconds{ 1000 }.count();
constexpr auto MAX_DURATION = std::chrono::milliseconds{ 5000 }.count();

struct NavigationMetric {
  int frame_rate;
  double error_m;
};
[[nodiscard]] auto toJSON(const NavigationMetric& log) -> std::string {
  return fmt::format(R"({{"frame_rate": {}, "error_m": {}}})", log.frame_rate, log.error_m);
}

struct ControlMetric {
  double error_m;
  int64_t elapsed_time;
  int frame_rate;
};
// NOLINTNEXTLINE
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ControlMetric, error_m, elapsed_time, frame_rate)

void runMotor() {
  auto mt = heph::random::createRNG();

  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  while (!heph::utils::TerminationBlocker::stopRequested()) {
    auto now = heph::telemetry::ClockT::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(mt)));
    heph::telemetry::metric("telemetry_example", "motor1",
                            MotorLog{
                                .status = heph::random::randomT<MotorStatus>(mt),
                                .current_amp = heph::random::randomT<double>(mt),
                                .velocity_rps = heph::random::randomT<int64_t>(mt),
                                .error_message = heph::random::randomT<std::string>(mt, 4),
                                .elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    heph::telemetry::ClockT::now() - now),
                                .counter = heph::random::randomT<uint32_t>(mt),
                                .temperature_celsius = -heph::random::randomT<int32_t>(mt),
                            });
  }
}

void runSLAM() {
  auto mt = heph::random::createRNG();
  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  while (!heph::utils::TerminationBlocker::stopRequested()) {
    heph::telemetry::metric("telemetry_example", "SLAM", "accuracy", heph::random::randomT<double>(mt));
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(mt)));
  }
}

void runNavigation() {
  auto mt = heph::random::createRNG();
  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  while (!heph::utils::TerminationBlocker::stopRequested()) {
    heph::telemetry::metric("telemetry_example", "Navigation",
                            NavigationMetric{
                                .frame_rate = heph::random::randomT<int>(mt),
                                .error_m = heph::random::randomT<double>(mt),
                            });
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(mt)));
  }
}

void runControll() {
  auto mt = heph::random::createRNG();
  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  while (!heph::utils::TerminationBlocker::stopRequested()) {
    heph::telemetry::metric("telemetry_example", "Controll",
                            ControlMetric{
                                .error_m = heph::random::randomT<double>(mt),
                                .elapsed_time = heph::random::randomT<int64_t>(mt),
                                .frame_rate = heph::random::randomT<int>(mt),
                            });
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(mt)));
  }
}

}  // namespace telemetry_example

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;
  const heph::utils::StackTrace stack;

  try {
    // Register the sinks.
    auto terminal_sink = heph::telemetry::createTerminalSink();
    heph::telemetry::registerSink(terminal_sink.get());
    auto rest_sink = heph::telemetry::createRESTSink({ .url = "http://127.0.0.1:5000" });
    heph::telemetry::registerSink(rest_sink.get());
    auto influxdb_sink = heph::telemetry::createInfluxDBSink(
        { .url = "localhost:8087", .token = "my-super-secret-auth-token", .database = "hephaestus" });
    heph::telemetry::registerSink(influxdb_sink.get());

    // Motor: demonstrates protobuf serializable metric
    auto motor = std::async(std::launch::async, &telemetry_example::runMotor);
    // SLAM: demonstrates single value metric
    auto slam = std::async(std::launch::async, &telemetry_example::runSLAM);
    // Navigation: demonstrates JSON serializable metric
    auto navigation = std::async(std::launch::async, &telemetry_example::runNavigation);
    // Navigation: demonstrates JSON serializable metric via nlohmann::json
    auto controll = std::async(std::launch::async, &telemetry_example::runControll);

    motor.get();
    slam.get();
    navigation.get();
    controll.get();
  } catch (std::exception& e) {
    fmt::println(stderr, "Execution terminated with exception: {}", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
