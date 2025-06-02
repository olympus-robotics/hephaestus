//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>

#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/concurrency/io_ring/timer.h"

namespace heph::concurrency {
struct Context;
struct ContextScheduleT {};
struct ContextScheduleAfterT {};

struct ContextScheduler {
  Context* self;

  template <typename TagT = ContextScheduleT>
  auto schedule() {
    return makeSenderExpression<TagT>(self);
  }

  template <typename Rep, typename Period, typename TagT = ContextScheduleAfterT>
  auto scheduleAfter(std::chrono::duration<Rep, Period> duration) {
    return makeSenderExpression<TagT>(std::tuple{ self, duration });
  }

  [[nodiscard]] static constexpr auto query(stdexec::get_forward_progress_guarantee_t /*unused*/) noexcept {
    return stdexec::forward_progress_guarantee::concurrent;
  }

  friend auto operator<=>(ContextScheduler const&, ContextScheduler const&) = default;
};

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
};

struct TaskBase;
struct TaskDispatchOperation {
  void handleCompletion() const;
  TaskBase* self;
};

struct TaskBase {
  virtual ~TaskBase() = default;
  virtual void start() noexcept = 0;
  virtual void setValue() noexcept = 0;
  virtual void setStopped() noexcept = 0;

  TaskDispatchOperation dispatch_operation{ this };
  TaskBase* next{ nullptr };
};

template <typename Receiver, typename Context>
struct Task : TaskBase {
  Context* context{ nullptr };
  Receiver receiver;
  stdexec::inplace_stop_token stop_token;

  Task(Context* context, Receiver&& receiver, stdexec::inplace_stop_token stop_token)
    : context(context), receiver(std::move(receiver)), stop_token(std::move(stop_token)) {
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

template <typename Receiver, typename Context>
struct TimedTask : TaskBase {
  Context* context{ nullptr };
  io_ring::TimerClock::time_point start_time;
  Receiver receiver;
  bool timeout_started{ false };

  template <typename Rep, typename Period>
  TimedTask(Context* context, std::chrono::duration<Rep, Period> start_after, Receiver&& receiver)
    : context(context), start_time(io_ring::TimerClock::now() + start_after), receiver(std::move(receiver)) {
  }

  void start() noexcept final {
    if (!timeout_started) {
      timeout_started = true;
      context->enqueueAt(this, start_time);
      return;
    }
    context->enqueue(this);
  }

  void setValue() noexcept final {
    stdexec::set_value(std::move(receiver));
  }

  void setStopped() noexcept final {
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
    return Task<std::decay_t<Receiver>, Context>{ context, std::move(receiver), context->getStopToken() };
  };

  static constexpr auto START = []<typename Receiver>(Task<std::decay_t<Receiver>, Context>& task,
                                                      Receiver& /*receiver*/) noexcept { task.start(); };
};

template <>
struct SenderExpressionImpl<ContextScheduleAfterT> : DefaultSenderExpressionImpl {
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
