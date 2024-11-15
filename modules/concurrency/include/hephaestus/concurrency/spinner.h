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
  using TransitionCallback = std::function<void()>;
  using PolicyCallback = std::function<bool()>;

  struct Callbacks {
    TransitionCallback init_cb = []() {};
    TransitionCallback spin_once_cb = []() {};
    TransitionCallback completion_cb = []() {};

    PolicyCallback shall_stop_spinning_cb = []() { return false; };  //!< Default: spin indefinitely.
    PolicyCallback shall_restart_cb = []() { return false; };        //!< Default: do not restart.
  };

  /// @brief Create a continuous spinner which cannot be stopped by the callback.
  /// Example: a callback which reads data from a sensor, until the spinner is stopped.
  explicit Spinner(Callbacks&& callbacks, double rate_hz = 0);

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

private:
  void spin();

private:
  enum class State : uint8_t { NOT_INITIALIZED, FAILED, READY_TO_SPIN, SPIN_SUCCESSFUL, EXIT };
  State state_ = State::NOT_INITIALIZED;
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
