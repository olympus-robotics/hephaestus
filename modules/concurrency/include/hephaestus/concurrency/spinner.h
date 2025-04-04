//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <limits>
#include <mutex>
#include <optional>

#include "hephaestus/concurrency/spinner_state_machine.h"

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

  /// @brief Create a callback for the spinner which internally handles the state machine.
  [[nodiscard]] static auto
  createCallbackWithStateMachine(spinner_state_machine::StateMachineCallbackT&& state_machine_callback)
      -> StoppableCallback;

  /// @brief Create a spinner with a stoppable callback. A stoppable callback is a function that returns
  /// SpinResult::STOP to indicate that the spinner should stop. Other types of callbacks are supported via
  /// mappings to StoppableCallback. Example: a callback that stops after 10 iterations.
  /// @param stoppable_callback The callback to be called in the spinner loop.
  /// @param spin_period The duration between spins. If not provided, the spinner will spin as fast as
  /// possible.
  /// @param component_name A unique name for this spinner for telemetry logging.
  explicit Spinner(StoppableCallback&& stoppable_callback,
                   std::optional<std::chrono::duration<double>> spin_period = std::nullopt,
                   std::optional<std::string> component_name = std::nullopt);

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
  std::optional<std::string> component_name_ = std::nullopt; //!< a unique name for this spinner for telemetry logging

  StoppableCallback stoppable_callback_;
  Callback termination_callback_ = []() {};

  std::atomic_bool stop_requested_ = false;
  std::future<void> async_spinner_handle_;
  std::atomic_flag spinner_completed_ = ATOMIC_FLAG_INIT;

  std::optional<std::chrono::duration<double>> spin_period_;
  std::mutex mutex_;
  std::condition_variable condition_;
};

namespace internal {
[[nodiscard]] auto computeNextSpinTimestamp(const std::chrono::system_clock::time_point& start_timestamp,
                                            const std::chrono::system_clock::time_point& now,
                                            std::chrono::duration<double> spin_period)
    -> std::chrono::system_clock::time_point;
}
}  // namespace heph::concurrency
