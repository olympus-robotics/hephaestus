
#include <atomic>
#include <chrono>
#include <csignal>

#include <exec/async_scope.hpp>
#include <exec/repeat_effect_until.hpp>

#include "hephaestus/concurrency/context.h"

namespace {
std::atomic<bool> stop{ false };

void setStop(int /*unused*/) {
  stop.store(true, std::memory_order_release);
}
}  // namespace
// Signal handler to stop execution...

auto main() -> int {
  static constexpr std::chrono::milliseconds PERIOD{ 10 };

  heph::concurrency::Context context{ {} };

  struct sigaction set_stop{};
  set_stop.sa_handler = setStop;
  sigaction(SIGINT, &set_stop, nullptr);
  sigaction(SIGTERM, &set_stop, nullptr);

  exec::async_scope scope;

  auto last_steady = std::chrono::steady_clock::now();
  auto last_system = std::chrono::system_clock::now();

  scope.spawn(exec::repeat_effect_until(
      context.scheduler().scheduleAfter(PERIOD) | stdexec::then([&context, &last_steady, &last_system] {
        auto now_steady = std::chrono::steady_clock::now();
        auto now_system = std::chrono::system_clock::now();

        auto duration_steady = now_steady - last_steady;
        auto duration_system = now_system - last_system;

        auto drift_steady = std::chrono::duration_cast<std::chrono::milliseconds>(PERIOD - duration_steady);
        auto drift_system = std::chrono::duration_cast<std::chrono::system_clock::duration>(duration_steady -
                                                                                            duration_system);

        fmt::println("{}: system clock drift {}, os scheduling drift {}", now_system, drift_steady,
                     drift_system);
        last_steady = now_steady;
        last_system = now_system;

        bool should_stop = stop.load(std::memory_order_acquire);
        if (should_stop) {
          context.requestStop();
        }
        return should_stop;
      })));

  context.run();
  fmt::println(stderr, "booyah");
}
