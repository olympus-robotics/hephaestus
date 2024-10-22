//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <thread>

namespace heph::concurrency {

/// A spinner is a class that spins in a loop calling a user-defined function.
/// If the function is blocking, the spinner will block the thread.
/// If the input `rate_hz` is set to a non-zero value, the spinner will call the user-defined function at
/// the given fixed rate.
class Spinner {
public:
  enum class SpinResult : bool { Continue, Stop };
  using Callback = std::function<SpinResult()>;

  explicit Spinner(Callback&& callback, double rate_hz = 0);
  ~Spinner();
  Spinner(const Spinner&) = delete;
  auto operator=(const Spinner&) -> Spinner& = delete;
  Spinner(Spinner&&) = delete;
  auto operator=(Spinner&&) -> Spinner& = delete;

  void start();
  auto stop() -> std::future<void>;
  void wait();

  [[nodiscard]] auto spinCount() const -> uint64_t;

private:
  void spin();
  void stopImpl();

private:
  Callback callback_;

  std::atomic_bool is_started_ = false;
  std::atomic_bool stop_requested_ = false;
  std::future<void> async_spinner_handle_;
  std::promise<SpinResult> spin_result_promise_;

  std::chrono::microseconds spin_period_;
  std::chrono::system_clock::time_point start_timestamp_;
  uint64_t spin_count_{ 0 };
  std::mutex mutex_;
  std::condition_variable condition_;
};
}  // namespace heph::concurrency
