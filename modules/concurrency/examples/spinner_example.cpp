//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <csignal>

#include <fmt/core.h>

#include "hephaestus/concurrency/spinner.h"

class TestSpinner : public heph::concurrency::Spinner {
public:
  std::atomic<int> counter = 0;

protected:
  void spinOnce() override {
    fmt::println("Spinning once. Counter: {}", counter.load());
    ++counter;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // NOLINT
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic_flag stop_called = ATOMIC_FLAG_INIT;
void handleSigint(int signal) {
  if (signal == SIGINT) {
    fmt::println("Stop called.");
    stop_called.test_and_set();
    stop_called.notify_all();
  }
}

auto main() -> int {
  try {
    TestSpinner spinner;
    std::ignore = std::signal(SIGINT, handleSigint);

    spinner.start();

    // Wait until stop_called is set
    stop_called.wait(false);

    auto future = spinner.stop();
    future.get();
  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
