//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/timing/watchdog.h"

#include <mutex>
#include <utility>

namespace heph::utils::timing {
void WatchdogTimer::start(std::chrono::milliseconds period, Callback&& callback) {
  period_ = period;
  callback_ = std::move(callback);
  running_ = true;
  pat_ = false;
  start_timestamp_ = std::chrono::system_clock::now();
  thread_ = std::thread([this]() { loop(); });
}

void WatchdogTimer::stop() {
  running_ = false;
  condition_.notify_all();
  if (thread_.joinable()) {
    thread_.join();
  }
}

void WatchdogTimer::pat() {
  pat_ = true;
}

void WatchdogTimer::loop() {
  while (running_) {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto target_timestamp = start_timestamp_ + (++fire_count_) * period_;
    if (condition_.wait_until(lock, target_timestamp, [this] { return !running_; })) {
      break;
    }

    if (pat_) {
      pat_ = false;
      continue;
    }

    callback_();
  }
}
}  // namespace heph::utils::timing
