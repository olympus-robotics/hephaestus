//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <functional>

namespace heph::utils::timing {

/// @class RateLimiter
/// @brief A class to limit the rate of function execution.
///
/// This class provides a mechanism to ensure that a function is not executed more frequently than a specified
/// interval. Might be handy to use with logging.
class RateLimiter final {
  using ClockT = std::chrono::steady_clock;

public:
  explicit RateLimiter(std::chrono::milliseconds period);

  void operator()(const std::function<void()>& callback);

private:
  std::chrono::milliseconds period_;
  /// Default: initialized to the epoch to ensure the first call is not rate-limited.
  std::chrono::time_point<ClockT, std::chrono::microseconds> timestamp_last_call_;
};

}  // namespace heph::utils::timing
