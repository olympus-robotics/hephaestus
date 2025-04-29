//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <exception>
#include <memory>
#include <utility>
#include <vector>

#include <exec/async_scope.hpp>
#include <exec/repeat_effect_until.hpp>
#include <fmt/base.h>
#include <fmt/format.h>
#include <hephaestus/telemetry/log_sink.h>
#include <stdexec/execution.hpp>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/concurrency/context.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/telemetry_influxdb_sink/influxdb_metric_sink.h"

namespace {
std::atomic<bool> stop{ false };  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// Signal handler to stop execution...
void setStop(int /*unused*/) {
  stop.store(true, std::memory_order_release);
}
}  // namespace

struct ClockJitter {
  std::chrono::milliseconds::rep period;
  std::chrono::microseconds::rep scheduler;
  std::chrono::microseconds::rep system_clock;
};
// NOLINTNEXTLINE(misc-include-cleaner)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(ClockJitter, scheduler, system_clock);

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

    heph::concurrency::Context context{ {} };

    struct sigaction set_stop{};             // NOLINT(misc-include-cleaner)
    set_stop.sa_handler = setStop;           // NOLINT(misc-include-cleaner)
    sigaction(SIGINT, &set_stop, nullptr);   // NOLINT(misc-include-cleaner)
    sigaction(SIGTERM, &set_stop, nullptr);  // NOLINT(misc-include-cleaner)

    exec::async_scope scope;

    std::vector<std::chrono::steady_clock::time_point> last_steady(PERIOD.size(),
                                                                   std::chrono::steady_clock::now());
    std::vector<std::chrono::system_clock::time_point> last_system(PERIOD.size(),
                                                                   std::chrono::system_clock::now());
    std::size_t id{ 0 };
    for (auto period : PERIOD) {
      scope.spawn(exec::repeat_effect_until(
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
                                        .period = period.count(),
                                        .scheduler = jitter_scheduling.count(),
                                        .system_clock = jitter_system_clock.count(),
                                    });
            last_steady = now_steady;
            last_system = now_system;

            const bool should_stop = stop.load(std::memory_order_acquire);
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
