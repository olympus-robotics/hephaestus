//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/timing/rate_limiter.h"

#include <chrono>
#include <functional>

namespace heph::utils::timing {

RateLimiter::RateLimiter(std::chrono::milliseconds period) : period_{ period } {
}

void RateLimiter::operator()(const std::function<void()>& callback) {
  auto now = ClockT::now();
  if (now - timestamp_last_call_ > period_) {
    callback();
    timestamp_last_call_ = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  }
}

}  // namespace heph::utils::timing
