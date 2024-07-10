//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <random>
#include <thread>
#include <tuple>

#include "example_proto_conversion.h"
#include "hephaestus/random/random_container.h"
#include "hephaestus/random/random_generator.h"
#include "hephaestus/random/random_type.h"
#include "hephaestus/telemetry/sink.h"
#include "hephaestus/telemetry/telemetry.h"
#include "hephaestus/utils/signal_handler.h"

namespace heph::telemetry::examples {

void run() {
  auto mt = random::createRNG();
  static constexpr auto MIN_DURATION = std::chrono::milliseconds{ 100 }.count();
  static constexpr auto MAX_DURATION = std::chrono::milliseconds{ 500 }.count();
  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  while (!heph::utils::TerminationBlocker::stopRequested()) {
    auto now = telemetry::ClockT::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(duration_dist(mt)));
    telemetry::Telemetry::log("example", "motor1",
                              MotorLog{
                                  .status = random::randomT<MotorStatus>(mt),
                                  .current_amp = random::randomT<double>(mt),
                                  .velocity_rps = random::randomT<double>(mt),
                                  .error_message = random::randomT<std::string>(mt, 4),
                                  .elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      telemetry::ClockT::now() - now),
                              });
  }
}

}  // namespace heph::telemetry::examples

auto main(int argc, const char* argv[]) -> int {
  (void)argc;
  (void)argv;
  try {
    auto terminal_sink = heph::telemetry::createTerminalSink();
    heph::telemetry::Telemetry::registerSink(terminal_sink.get());

    heph::telemetry::examples::run();
  } catch (std::exception& e) {
    std::ignore = std::fputs(e.what(), stderr);
  }
  return EXIT_SUCCESS;
}
