//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <fmt/base.h>
#include <fmt/format.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/telemetry_influxdb_sink/influxdb_metric_sink.h"
#include "hephaestus/utils/signal_handler.h"

struct ClockJitter {
  std::chrono::milliseconds::rep period_ms;
  std::chrono::microseconds::rep scheduler_us;
  std::chrono::microseconds::rep system_clock_us;
};
// NOLINTNEXTLINE(misc-include-cleaner)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(ClockJitter, scheduler_us, system_clock_us);

class Spinner : public heph::conduit::Node<Spinner> {
  std::chrono::milliseconds spin_period_;

  std::chrono::steady_clock::time_point last_steady_;
  std::chrono::system_clock::time_point last_system_;
  std::string tag_ = fmt::format("period={}", spin_period_);

  bool output_{ false };

public:
  explicit Spinner(std::chrono::milliseconds period)
    : Node()
    , spin_period_(period)
    , last_steady_(std::chrono::steady_clock::now())
    , last_system_(std::chrono::system_clock::now()) {
  }

  [[nodiscard]] auto period() const {
    return spin_period_;
  }

  void toggleOutput() {
    output_ = !output_;
  }

  void operator()() {
    auto now_steady = std::chrono::steady_clock::now();
    auto now_system = std::chrono::system_clock::now();

    auto duration_steady = now_steady - last_steady_;
    auto duration_system = now_system - last_system_;

    // a positive duration drift indicates that the clock under consideration took longer than
    // expected, and vice versa
    auto jitter_scheduling =
        std::chrono::duration_cast<std::chrono::microseconds>(duration_steady - period());
    auto jitter_system_clock =
        std::chrono::duration_cast<std::chrono::microseconds>(duration_system - duration_steady);

    if (output_) {
      heph::log(heph::INFO, "", "scheduling", jitter_scheduling, "clock", jitter_system_clock);
    }
    heph::telemetry::record("conduit_clock_jitter", tag_,
                            ClockJitter{
                                .period_ms = period().count(),
                                .scheduler_us = jitter_scheduling.count(),
                                .system_clock_us = jitter_system_clock.count(),
                            });
    last_steady_ = now_steady;
    last_system_ = now_system;
  }
};

auto main(int argc, const char* argv[]) -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc =
        heph::cli::ProgramDescription("Determin clock drift over time with different spinner periods");
    desc.defineOption<std::string>("influxdb_host", "Hostname of the influxdb instance to log data to",
                                   "localhost:8099")
        .defineOption<std::string>("influxdb_token", "Access token for influxdb",
                                   "my-super-secret-auth-token")
        .defineOption<std::string>("influxdb_database", "influxdb database for the measurements",
                                   "hephaestus");
    const auto args = std::move(desc).parse(argc, argv);

    using namespace std::chrono_literals;
    // NOLINTNEXTLINE(misc-include-cleaner)
    static constexpr std::array PERIOD{ 1ms, 10ms, 100ms, 200ms, 500ms };

    static constexpr auto TELEMETRY_PERIOD = std::chrono::duration<double>(1.0);

    auto influxdb_sink = heph::telemetry_sink::InfluxDBSink::create(
        { .url = args.getOption<std::string>("influxdb_host"),
          .token = args.getOption<std::string>("influxdb_token"),
          .database = args.getOption<std::string>("influxdb_database"),
          .flush_period = TELEMETRY_PERIOD });
    heph::telemetry::registerMetricSink(std::move(influxdb_sink));

    heph::conduit::NodeEngine engine{ {} };

    std::vector<Spinner> spinners;
    spinners.reserve(PERIOD.size());

    for (auto period : PERIOD) {
      spinners.emplace_back(period);
      engine.addNode(spinners.back());
    }
    spinners.back().toggleOutput();

    heph::utils::TerminationBlocker::registerInterruptCallback([&engine] { engine.requestStop(); });
    engine.run();
    fmt::println(stderr, "");
  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return 1;
  }

  return 0;
}
