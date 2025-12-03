//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

#include <exec/async_scope.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/repeat_until.h"
#include "hephaestus/telemetry/influxdb_sink/influxdb_metric_sink.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/log_sink.h"
#include "hephaestus/telemetry/log/sinks/absl_sink.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/utils/signal_handler.h"

struct ClockJitter {
  std::chrono::milliseconds::rep period_ms;
  std::chrono::microseconds::rep scheduler_us;
  std::chrono::microseconds::rep system_clock_us;
};

auto main(int argc, const char* argv[]) -> int {
  try {
    heph::telemetry::makeAndRegisterLogSink<heph::telemetry::AbslLogSink>();

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
    static constexpr std::array PERIOD{ 1ms, 10ms, 100ms, 200ms, 500ms };

    static constexpr auto TELEMETRY_PERIOD = std::chrono::duration<double>(1.0);

    auto influxdb_sink = heph::telemetry_sink::InfluxDBSink::create(
        { .url = args.getOption<std::string>("influxdb_host"),
          .token = args.getOption<std::string>("influxdb_token"),
          .database = args.getOption<std::string>("influxdb_database"),
          .flush_period = TELEMETRY_PERIOD });
    heph::telemetry::registerMetricSink(std::move(influxdb_sink));

    heph::concurrency::Context context{ {} };

    exec::async_scope scope;

    std::vector<std::chrono::steady_clock::time_point> last_steady(PERIOD.size(),
                                                                   std::chrono::steady_clock::now());
    std::vector<std::chrono::system_clock::time_point> last_system(PERIOD.size(),
                                                                   std::chrono::system_clock::now());
    std::size_t id{ 0 };
    for (auto period : PERIOD) {
      scope.spawn(heph::concurrency::repeatUntil(
          context.scheduler().scheduleAfter(period) |
          stdexec::then([period, tag = fmt::format("period={}", period), &last_steady = last_steady[id],
                         &last_system = last_system[id], &context] {
            auto now_steady = std::chrono::steady_clock::now();
            auto now_system = std::chrono::system_clock::now();

            auto duration_steady = now_steady - last_steady;
            auto duration_system = now_system - last_system;

            // a positive duration drift indicates that the clock under consideration took longer than
            // expected, and vice versa
            auto jitter_scheduling =
                std::chrono::duration_cast<std::chrono::microseconds>(duration_steady - period);
            auto jitter_system_clock =
                std::chrono::duration_cast<std::chrono::microseconds>(duration_system - duration_steady);

            if (period == PERIOD.back()) {
              heph::log(heph::INFO, "", "scheduling", jitter_scheduling, "clock", jitter_system_clock);
            }
            heph::telemetry::record("clock_jitter", tag,
                                    ClockJitter{
                                        .period_ms = period.count(),
                                        .scheduler_us = jitter_scheduling.count(),
                                        .system_clock_us = jitter_system_clock.count(),
                                    });
            last_steady = now_steady;
            last_system = now_system;

            const bool should_stop = heph::utils::TerminationBlocker::stopRequested();
            if (should_stop) {
              context.requestStop();
            }
            return should_stop;
          })));
      ++id;
    }

    context.run();
    fmt::println(stderr, "");
  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return 1;
  }

  return 0;
}
