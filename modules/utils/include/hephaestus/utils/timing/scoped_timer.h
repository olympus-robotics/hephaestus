//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <functional>

namespace heph::utils::timing {

/// ScopedTimer starts a timer upon construction and stops it upon destruction.
/// The elapsed time is passed to the provided callback function.
class ScopedTimer {
public:
  using ClockT = std::chrono::steady_clock;
  using Callback = std::function<void(ClockT::duration)>;
  using NowFunctionPtr = std::chrono::steady_clock::time_point (*)();

  explicit ScopedTimer(Callback&& callback, NowFunctionPtr now_fn = &ClockT::now)
    : callback_(std::move(callback)), start_timestamp_(now_fn()), now_fn_(now_fn) {
  }

  ~ScopedTimer() {
    const auto end_timestamp = now_fn_();
    callback_(end_timestamp - start_timestamp_);
  }

  ScopedTimer(const ScopedTimer&) = default;
  ScopedTimer(ScopedTimer&&) = delete;
  auto operator=(const ScopedTimer&) -> ScopedTimer& = default;
  auto operator=(ScopedTimer&&) -> ScopedTimer& = delete;

private:
  void loop();

private:
  Callback callback_{ nullptr };
  ClockT::time_point start_timestamp_;

  NowFunctionPtr now_fn_{ nullptr };
};

}  // namespace heph::utils::timing
