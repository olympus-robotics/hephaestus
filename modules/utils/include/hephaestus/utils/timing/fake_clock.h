//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>

namespace heph::utils::timing {

class FakeClock {
public:
  using duration = std::chrono::steady_clock::duration;
  using rep = std::chrono::steady_clock::rep;
  using period = std::chrono::steady_clock::period;
  using time_point = std::chrono::time_point<FakeClock>;

  ~FakeClock() = default;
  FakeClock(const FakeClock&) = delete;
  auto operator=(const FakeClock&) -> FakeClock& = delete;
  FakeClock(FakeClock&&) = delete;
  auto operator=(FakeClock&&) -> FakeClock& = delete;

  // Standard clock interface methods
  static auto now() -> time_point {
    return current_time;
  }

  // Static methods to control the mock clock
  static void reset() {
    current_time = time_point::min();
  }

  // Set the current time manually
  static void setCurrentTime(time_point new_time) {
    current_time = new_time;
  }

  // Advance the current time by a specific duration
  static void advance(duration delta) {
    current_time += delta;
  }

public:
  static constexpr bool is_steady = true;  // NOLINT(readability-identifier-naming)

private:
  // Private constructor to prevent instantiation
  FakeClock() = default;

  // Static variable to track current time
  static inline time_point current_time = time_point::min();
};
}  // namespace heph::utils::timing
