//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/spinner.h"

class TestSpinner : public heph::concurrency::Spinner {
public:
  std::atomic<int> counter = 0;

protected:
  void spinOnce() override {
    ++counter;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // simulate work
  }
};

auto main() -> int {
  try {
    TestSpinner spinner;

    spinner.start();

  } catch (const std::exception& ex) {
    fmt::println("{}", ex.what());
    return 1;
  }

  return 0;
}
