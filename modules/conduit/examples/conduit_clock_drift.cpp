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
#include "hephaestus/conduit/node_handle.h"
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

class SpinnerOperation {
public:
  explicit SpinnerOperation(std::chrono::milliseconds period) : spin_period_(period) {
  }

  void toggleOutput() {
    output_ = !output_;
  }

  [[nodiscard]] auto period() const {
    return spin_period_;
  }

  void update() const {
    if (output_) {
      heph::log(heph::INFO, "tick");
    }
  }

private:
  std::chrono::milliseconds spin_period_;

  bool output_{ false };
};

struct Spinner : heph::conduit::Node<Spinner, SpinnerOperation> {
  static auto name(const Spinner& self) -> std::string {
    return fmt::format("Spinner({})", self.data().period());
  }
  static auto period(const Spinner& self) {
    return self.data().period();
  }

  static void execute(Spinner& self) {
    self.data().update();
  }
};

auto main(int argc, const char* argv[]) -> int {
  try {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    auto desc =
        heph::cli::ProgramDescription("Determine clock drift over time with different spinner periods");
    desc.defineOption<std::string>("influxdb_host", "Hostname of the influxdb instance to log data to",
                                   "localhost:8099")
        .defineOption<std::string>("influxdb_token", "Access token for influxdb",
                                   "my-super-secret-auth-token")
        .defineOption<std::string>("influxdb_database", "influxdb database for the measurements",
                                   "hephaestus");
    const auto args = std::move(desc).parse(argc, argv);

    using namespace std::chrono_literals;
    // NOLINTNEXTLINE(misc-include-cleaner)
    static constexpr std::array PERIOD{ 1ms, 10ms, 20ms, 25ms, 30ms, 40ms, 100ms };

    static constexpr auto TELEMETRY_PERIOD = std::chrono::duration<double>(1.0);

    auto influxdb_sink = heph::telemetry_sink::InfluxDBSink::create(
        { .url = args.getOption<std::string>("influxdb_host"),
          .token = args.getOption<std::string>("influxdb_token"),
          .database = args.getOption<std::string>("influxdb_database"),
          .flush_period = TELEMETRY_PERIOD });
    heph::telemetry::registerMetricSink(std::move(influxdb_sink));

    heph::conduit::NodeEngine engine{ {} };

    std::vector<heph::conduit::NodeHandle<Spinner>> spinners;
    spinners.reserve(PERIOD.size());

    for (auto period : PERIOD) {
      spinners.emplace_back(engine.createNode<Spinner>(period));
    }
    spinners.back()->data().toggleOutput();

    heph::utils::TerminationBlocker::registerInterruptCallback([&engine] { engine.requestStop(); });
    engine.run();
    fmt::println(stderr, "");
  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return 1;
  }

  return 0;
}
