//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <atomic>
#include <cstdlib>
#include <exception>

#include <fmt/core.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/utils/signal_handler.h"

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

auto main() -> int {
  try {
    TestSpinner spinner;

    spinner.start();

    // Wait until signal is set
    heph::utils::TerminationBlocker::waitForInterrupt();

    spinner.stop().get();

  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
