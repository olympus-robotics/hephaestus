//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

namespace heph::concurrency {

class Spinner {
public:
  Spinner();
  virtual ~Spinner();
  void start();
  void stop();
  virtual void spin();
  virtual void spinOnce() = 0;  // Pure virtual function
  void addStopCallback(std::function<void> callback);

private:
  std::atomic<bool> is_started_ = false;
  std::function<void> stop_callback_;
  std::optional<std::stop_source> stop_token_;
  std::jthread spinner_thread_;
};
}  // namespace heph::concurrency
