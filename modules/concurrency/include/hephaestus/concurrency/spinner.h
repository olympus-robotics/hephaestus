//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>

namespace heph::concurrency {

struct SpinnerCallbacks {
  std::function<void() init_cb = []() {};
  std::function<void() spin_once_cb = []() {};
  std::function<void() termination_cb = []() {};

  std::function<bool> shall_stop_cb = []() { return false; };    //!< Default: spin indefinitely.
  std::function<bool> shall_re_init_cb = []() { return false; }; //!< Default: do not re-init.
};

/// A spinner is a class that spins in a loop calling a user-defined function.
/// If the function is blocking, the spinner will block the thread.
/// If the input `rate_hz` is set to a non-zero value, the spinner will call the user-defined function at
/// the given fixed rate.
class Spinner {
public:
  enum class SpinnerState : uint8_t {
    NOT_INITIALIZED,
    INITIALIZING,
    INIT_FAILED,
    READY_TO_SPIN,
    SPINNING,
    SPIN_FAILED,
    SPIN_SUCCESSFUL,
    TERMINATE,
    TERMINATING,
    TERMINATED
  };
  
  /// @brief Create a continuous spinner which cannot be stopped by the callback.
  /// Example: a callback which reads data from a sensor, until the spinner is stopped.
  explicit Spinner(SpinnerCallbacks&& callbacks, double rate_hz = 0);

  ~Spinner();
  Spinner(const Spinner&) = delete;
  auto operator=(const Spinner&) -> Spinner& = delete;
  Spinner(Spinner&&) = delete;
  auto operator=(Spinner&&) -> Spinner& = delete;

  void start();
  auto stop() -> std::future<void>;
  void wait();

private:
  using ErrorMessageT = std::string;
  [[nodiscard]] auto init() -> std::optional<ErrorMessageT>;
  [[nodiscard]] auto spinOnce() -> std::optional<ErrorMessageT>;
  void spin();
  void terminate();

private:
  SpinnerState state_ = SpinnerState::NOT_INITIALIZED;
  SpinnerCallbacks callbacks_;

  std::atomic_bool stop_requested_ = false;
  std::future<void> async_spinner_handle_;
  std::atomic_flag spinner_completed_ = ATOMIC_FLAG_INIT;

  std::chrono::microseconds spin_period_;
  std::chrono::system_clock::time_point start_timestamp_;
  std::mutex mutex_;
  std::condition_variable condition_;
};
}  // namespace heph::concurrency
