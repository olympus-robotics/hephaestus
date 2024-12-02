//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>

namespace heph::utils::timing {

class WatchdogTimer {
public:
  using Callback = std::function<void()>;

  /// Start the watchdog timer with the specified period and callback.
  /// Once the timer starts it will loop indefinitely, calling the callback ever `period` milliseconds.
  void start(std::chrono::milliseconds period, Callback&& callback);

  /// Stop the timer.
  void stop();

  /// Pat the watchdog. If called the timer will not call the callback for the next period.
  /// This can be used to implement a dead-man switch.
  void pat();

private:
  void loop();

private:
  std::chrono::system_clock::time_point start_timestamp_;
  uint64_t fire_count_{ 0 };
  std::chrono::milliseconds period_{ 0 };
  Callback callback_{ nullptr };

  std::atomic<bool> running_{ false };
  std::atomic<bool> pat_{ false };
  std::mutex mutex_;
  std::condition_variable condition_;
  std::thread thread_;
};

}  // namespace heph::utils::timing
