//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <condition_variable>
#include <csignal>
#include <mutex>

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

std::condition_variable cv;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex cv_m;             // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
void handleSigint(int signal) {
  if (signal == SIGINT) {
    fmt::println("Stop called.");
    cv.notify_one();
  }
}

auto main() -> int {
  try {
    TestSpinner spinner;
    std::ignore = std::signal(SIGINT, handleSigint);

    spinner.start();

    std::unique_lock<std::mutex> lock(cv_m);
    cv.wait(lock);
    spinner.stop();
  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
