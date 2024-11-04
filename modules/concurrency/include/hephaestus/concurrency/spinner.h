//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>

namespace heph::concurrency {

/// A spinner is a class that spins in a loop calling a user-defined function.
/// If the function is blocking, the spinner will block the thread.
/// If the input `rate_hz` is set to a non-zero value, the spinner will call the user-defined function at
/// the given fixed rate.
class Spinner {
public:
  enum class SpinResult : bool { CONTINUE, STOP };
  using StoppableCallback = std::function<SpinResult()>;
  using Callback = std::function<void()>;

  /// @brief Create a spinner with a stoppable callback. A stoppable callback is a function that returns
  /// SpinResult::STOP to indicate that the spinner should stop.
  /// Example: a callback that stops after 10 iterations.
  explicit Spinner(StoppableCallback&& stoppable_callback, double rate_hz = 0);

  /// @brief Create a continuous spinner which cannot be stopped by the callback.
  /// Example: a callback which reads data from a sensor, until the spinner is stopped.
  explicit Spinner(Callback&& callback, double rate_hz = 0);

  ~Spinner();
  Spinner(const Spinner&) = delete;
  auto operator=(const Spinner&) -> Spinner& = delete;
  Spinner(Spinner&&) = delete;
  auto operator=(Spinner&&) -> Spinner& = delete;

  void start();
  auto stop() -> std::future<void>;
  void wait();

  /// @brief  Set a callback that will be called when the spinner is stopped.
  /// This callback could be extendend to pass the reason why the spinner was stopped, e.g. exceptions, ...
  void setTerminationCallback(Callback&& termination_callback);

  [[nodiscard]] auto spinCount() const -> uint64_t;

private:
  void spin();
  void stopImpl();

private:
  StoppableCallback stoppable_callback_;
  Callback termination_callback_ = []() {};

  std::atomic_bool stop_requested_ = false;
  std::future<void> async_spinner_handle_;
  std::atomic_flag spinner_completed_ = ATOMIC_FLAG_INIT;

  std::chrono::microseconds spin_period_;
  std::chrono::system_clock::time_point start_timestamp_;
  uint64_t spin_count_{ 0 };
  std::mutex mutex_;
  std::condition_variable condition_;
};
}  // namespace heph::concurrency
