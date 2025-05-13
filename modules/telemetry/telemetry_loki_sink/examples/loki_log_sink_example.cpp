//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <thread>

#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/telemetry_loki_sink/loki_log_sink.h"
#include "hephaestus/utils/signal_handler.h"
#include "hephaestus/utils/stack_trace.h"

// To start a Grafana + Loki stack follow this instructions
// https://grafana.com/docs/plugins/grafana-lokiexplore-app/latest/access/#test-with-docker-compose

auto main(int /*unused*/, const char* /*unused*/[]) -> int {
  try {
    using namespace std::literals::string_literals;
    const heph::utils::StackTrace stack;

    heph::telemetry::LokiLogSinkConfig config;
    config.domain = "forkify";
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::LokiLogSink>(config));

    heph::log(heph::WARN, "testing loki log sink"s);

    const int num = 1234;
    heph::log(heph::INFO, "loki log sink with fields", "num", num, "quoted_string", "test");

    heph::log(heph::DEBUG, "debug loki debug log sink with fields", "num", num);

    heph::log(heph::TRACE, "debug loki trace log sink with fields", "num", num);

    heph::log(heph::ERROR, "debug loki error log sink with fields", "num", num);

    std::size_t counter = 0;
    while (!heph::utils::TerminationBlocker::stopRequested()) {
      static constexpr auto PERIOD = std::chrono::milliseconds{ 200 };
      heph::log(heph::INFO, "loki log sink with fields", "counter", counter);
      heph::log(heph::ERROR, "debug loki error log sink with fields", "counter", counter++);
      std::this_thread::sleep_for(PERIOD);
    }

  } catch (std::exception& e) {
    return 1;
  }

  return 0;
}
