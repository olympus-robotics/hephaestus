//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <functional>
#include <thread>

namespace heph::concurrency {

class Spinner {
public:
  Spinner();
  virtual ~Spinner();
  Spinner(const Spinner&) = delete;
  auto operator=(const Spinner&) -> Spinner& = delete;
  Spinner(Spinner&&) = delete;
  auto operator=(Spinner&&) -> Spinner& = delete;

  void start();
  void stop();
  virtual void spin();
  virtual void spinOnce() = 0;  // Pure virtual function
  void addStopCallback(std::function<void()> callback);

private:
  std::atomic_bool is_started_ = false;
  std::atomic_bool stop_requested_ = false;
  std::function<void()> stop_callback_;
  std::thread spinner_thread_;
};
}  // namespace heph::concurrency
