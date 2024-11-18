//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstdlib>
#include <exception>
#include <future>

#include <fmt/core.h>

#include "hephaestus/concurrency/spinner.h"
#include "hephaestus/utils/signal_handler.h"

static constexpr auto RATE_HZ = 10;

class Worker {
public:
  Worker()
    : spinner_(heph::concurrency::Spinner::createNeverStoppingCallback([this] { doWork(); }), RATE_HZ) {
  }

  void start() {
    spinner_.start();
  }

  [[nodiscard]] auto stop() -> std::future<void> {
    return spinner_.stop();
  }

private:
  void doWork() {
    fmt::println("Spinning once. Counter: {}", counter_++);
  }

private:
  heph::concurrency::Spinner spinner_;
  std::size_t counter_ = 0;
};

auto main() -> int {
  try {
    Worker worker;

    worker.start();

    // Wait until signal is set
    heph::utils::TerminationBlocker::waitForInterrupt();

    worker.stop().get();

  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
