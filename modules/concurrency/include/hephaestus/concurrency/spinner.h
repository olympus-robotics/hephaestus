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
/// If the input `rate_hz` is set to a non-infinite value, the spinner will call the user-defined function at
/// the given fixed rate. The spinner behavior can be configured using callbacks.
class Spinner {
public:
  enum class SpinResult : bool { CONTINUE, STOP };
  using StoppableCallback = std::function<SpinResult()>;
  using Callback = std::function<void()>;

  /// @brief Wrap the user provided callback in a callback that never stops.
  [[nodiscard]] static auto createNeverStoppingCallback(Callback&& callback) -> StoppableCallback;

  struct StateMachineCallbacks {
    using TransitionCallback = std::function<void()>;
    using PolicyCallback = std::function<bool()>;

    TransitionCallback init_cb = []() {};       //!< Handles initialization.
    TransitionCallback spin_once_cb = []() {};  //!< Handles execution.

    PolicyCallback shall_stop_spinning_cb = []() { return false; };  //!< Default: spin indefinitely.
    PolicyCallback shall_restart_cb = []() { return false; };        //!< Default: do not restart.
  };

  /// @brief Create a callback for the spinner which internally handles the state machine.
  [[nodiscard]] static auto createCallbackWithStateMachine(StateMachineCallbacks&& callbacks)
      -> StoppableCallback;

  /// @brief Create a spinner with a stoppable callback. A stoppable callback is a function that returns
  /// SpinResult::STOP to indicate that the spinner should stop. Other types of callbacks are supported via
  /// mappings to StoppableCallback. Example: a callback that stops after 10 iterations.
  explicit Spinner(StoppableCallback&& stoppable_callback,
                   double rate_hz = std::numeric_limits<double>::infinity());

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
  void terminate();

private:
  StoppableCallback stoppable_callback_;
  Callback termination_callback_ = []() {};

  std::atomic_bool stop_requested_ = false;
  std::future<void> async_spinner_handle_;
  std::atomic_flag spinner_completed_ = ATOMIC_FLAG_INIT;

  std::chrono::microseconds spin_period_;
  std::mutex mutex_;
  std::condition_variable condition_;
};
}  // namespace heph::concurrency
