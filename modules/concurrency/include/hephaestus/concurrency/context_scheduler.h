//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <exception>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__tag_invoke.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"
#include "hephaestus/concurrency/io_ring/timer.h"

namespace heph::concurrency {
class Context;
struct ContextScheduleT {};
struct ContextScheduleAtT {};

struct ContextScheduler {
  Context* self;

  [[nodiscard]] auto context() const -> Context& {
    return *self;
  }

  template <typename TagT = ContextScheduleT>
  auto schedule() {
    return makeSenderExpression<TagT>(self);
  }

  template <typename Rep, typename Period, typename TagT = ContextScheduleAtT>
  auto scheduleAfter(std::chrono::duration<Rep, Period> duration) {
    return scheduleAt(io_ring::TimerClock::now() + duration);
  }

  template <typename Clock, typename Duration, typename TagT = ContextScheduleAtT>
  auto scheduleAt(std::chrono::time_point<Clock, Duration> time_point) {
    return makeSenderExpression<TagT>(std::tuple{ self, time_point });
  }

  [[nodiscard]] static constexpr auto query(stdexec::get_forward_progress_guarantee_t /*unused*/) noexcept {
    return stdexec::forward_progress_guarantee::concurrent;
  }

  friend auto operator<=>(const ContextScheduler&, const ContextScheduler&) = default;
};

struct GetContextT : stdexec::__query<GetContextT> {
  static constexpr auto query(stdexec::forwarding_query_t /*unused*/) noexcept -> bool {
    return true;
  }
  template <typename Env>
    requires stdexec::tag_invocable<GetContextT, const Env&>
  auto operator()(const Env& env) const noexcept -> stdexec::tag_invoke_result_t<GetContextT, const Env&> {
    static_assert(stdexec::nothrow_tag_invocable<GetContextT, const Env&>);
    return stdexec::tag_invoke(GetContextT{}, env);
  }

  template <typename Tag = GetContextT>
  auto operator()() const noexcept;
};

// NOLINTNEXTLINE (readability-identifier-naming)
static constexpr GetContextT getContext{};

struct ContextEnv {
  Context* self;

  [[nodiscard]] static constexpr auto query(stdexec::__is_scheduler_affine_t /*ignore*/) noexcept {
    return true;
  }

  [[nodiscard]] constexpr auto
  query(stdexec::get_completion_scheduler_t<stdexec::set_value_t> /*ignore*/) const noexcept
      -> ContextScheduler {
    return { self };
  }

  [[nodiscard]] auto query(stdexec::get_stop_token_t /*ignore*/) const noexcept
      -> stdexec::inplace_stop_token;

  [[nodiscard]] auto query(GetContextT /*ignore*/) const noexcept -> Context&;
};

struct TaskBase;
struct TaskDispatchOperation : io_ring::IoRingOperationBase {
  explicit TaskDispatchOperation(TaskBase* task) noexcept : self(task) {
  }

  void handleCompletion(::io_uring_cqe* cqe) final;
  TaskBase* self;
};

struct TaskBase {
  virtual ~TaskBase() = default;
  virtual void start() noexcept = 0;
  virtual void setValue() noexcept = 0;
  virtual void setStopped() noexcept = 0;

  TaskDispatchOperation dispatch_operation{ this };
  TaskBase* next{ nullptr };
  TaskBase* prev{ nullptr };
};

template <typename Receiver, typename Context>
struct Task : TaskBase {
  Context* context{ nullptr };
  Receiver receiver;

  Task(Context* context_input, Receiver&& receiver_input)
    : context(context_input), receiver(std::move(receiver_input)) {
  }

  void start() noexcept final {
    context->enqueue(this);
  }

  void setValue() noexcept final {
    stdexec::set_value(std::move(receiver));
  }

  void setStopped() noexcept final {
    stdexec::set_stopped(std::move(receiver));
  }
};

template <typename ReceiverT, typename ContextT>
struct TimedTask : TaskBase {
  struct StopCallback {
    void operator()() const noexcept {
      self->setStopped();
    }
    TimedTask* self;
  };
  using ReceiverEnvT = stdexec::env_of_t<ReceiverT>;
  using StopTokenT = stdexec::stop_token_of_t<ReceiverEnvT>;
  using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, StopCallback>;

  ContextT* context{ nullptr };
  io_ring::TimerClock::time_point start_time;
  ReceiverT receiver;
  std::optional<StopCallbackT> stop_callback;
  bool timeout_started{ false };

  template <typename Clock, typename Duration>
  TimedTask(Context* context_input, std::chrono::time_point<Clock, Duration> start_time_input,
            ReceiverT&& receiver_input)
    : context(context_input), start_time(start_time_input), receiver(std::move(receiver_input)) {
    // Avoid putting it the task in the timer when the deadline was already exceeded...
    if (start_time <= io_ring::TimerClock::now()) {
      timeout_started = true;
    }
  }

  void start() noexcept final {
    if (!timeout_started) {
      auto stop_token = stdexec::get_stop_token(stdexec::get_env(receiver));
      stop_callback.emplace(stop_token, StopCallback{ this });
      timeout_started = true;
      context->enqueueAt(this, start_time);
      return;
    }
    context->enqueue(this);
  }

  void setValue() noexcept final {
    stop_callback.reset();
    stdexec::set_value(std::move(receiver));
  }

  void setStopped() noexcept final {
    context->dequeueTimer(this);
    stop_callback.reset();
    stdexec::set_stopped(std::move(receiver));
  }
};

template <>
struct SenderExpressionImpl<ContextScheduleT> : DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = [](Ignore, Ignore = {}) noexcept {
    return stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                          stdexec::set_stopped_t()>{};
  };

  static constexpr auto GET_ATTRS = [](Context* context) noexcept -> ContextEnv { return { context }; };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver& receiver) {
    auto [_, context] = std::forward<Sender>(sender);
    return Task<std::decay_t<Receiver>, Context>{ context, std::move(receiver) };
  };

  static constexpr auto START = []<typename Receiver>(Task<std::decay_t<Receiver>, Context>& task,
                                                      Receiver& /*receiver*/) noexcept { task.start(); };
};

template <>
struct SenderExpressionImpl<ContextScheduleAtT> : DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = [](Ignore, Ignore = {}) noexcept {
    return stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                          stdexec::set_stopped_t()>{};
  };

  static constexpr auto GET_ATTRS = [](auto& data) noexcept -> ContextEnv { return { std::get<0>(data) }; };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver& receiver) {
    auto [_, data] = std::forward<Sender>(sender);
    auto* context = std::get<0>(data);
    return TimedTask<std::decay_t<Receiver>, Context>{ context, std::get<1>(data), std::move(receiver) };
  };

  static constexpr auto START = []<typename Receiver>(TimedTask<std::decay_t<Receiver>, Context>& task,
                                                      Receiver& /*receiver*/) noexcept { task.start(); };
};
}  // namespace heph::concurrency
