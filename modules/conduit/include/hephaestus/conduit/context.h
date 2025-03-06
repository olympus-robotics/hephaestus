
//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <cerrno>
#include <chrono>
#include <ctime>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <hephaestus/conduit/completion_handler.h>
#include <hephaestus/conduit/ticker.h>
#include <liburing.h>
#include <stdexec/execution.hpp>

namespace heph::conduit {

struct ContextOptions {
  double time_scale_factor{ 1.0 };
};

struct ScheduleT {};

struct TimedScheduleT {};
}  // namespace heph::conduit
namespace stdexec {
template <>
struct __sexpr_impl<heph::conduit::ScheduleT> : stdexec::__sexpr_defaults {
  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  static constexpr auto get_completion_signatures = [](stdexec::__ignore, stdexec::__ignore = {}) noexcept
      -> stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t()> { return {}; };

  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  static constexpr auto get_state = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver& receiver) {
    static_assert(stdexec::sender_expr_for<Sender, heph::conduit::ScheduleT>);
    return sender.apply(
        std::forward<Sender>(sender),
        [&]<typename Context>(stdexec::__ignore, Context* context) { return context->createTask(receiver); });
  };
  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  static constexpr auto start = [](auto& task, stdexec::__ignore) { task.start(); };
};

template <>
struct __sexpr_impl<heph::conduit::TimedScheduleT> : stdexec::__sexpr_defaults {
  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  static constexpr auto get_completion_signatures = [](stdexec::__ignore, stdexec::__ignore = {}) noexcept
      -> stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t()> { return {}; };

  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  static constexpr auto get_state = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver& receiver) {
    static_assert(stdexec::sender_expr_for<Sender, heph::conduit::TimedScheduleT>);
    return sender.apply(std::forward<Sender>(sender), [&]<typename Tuple>(stdexec::__ignore, Tuple data) {
      auto* context = stdexec::__tup::get<0>(data);
      auto duration = stdexec::__tup::get<1>(data);
      return context->createTimedTask(receiver, duration);
    });
  };
  // NOLINTNEXTLINE(readability-identifier-naming) - wrapping stdexec interface
  static constexpr auto start = [](auto& task, stdexec::__ignore) { task.start(); };
};
}  // namespace stdexec
namespace heph::conduit {

class Context {
public:
  explicit Context(ContextOptions options) : ring_(), ticker_(this, options.time_scale_factor) {
    static constexpr std::size_t NUM_ENTRIES = 1024;
    // FIXME: error handling
    io_uring_queue_init(NUM_ENTRIES, &ring_, IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_SINGLE_ISSUER);
    // FIXME: error handling
    io_uring_register_ring_fd(&ring_);
  }
  Context() : Context(ContextOptions{}) {
  }

  template <typename F>
  void scheduleTask(F /*f*/) {
  }

  void run() {
    io_uring_submit(&ring_);
    executeTasks();
    while (isRunning()) {
      int res = io_uring_submit_and_wait(&ring_, 1);
      if (res != 0) {
        // FIXME: error handling
        if (-res == EAGAIN || -res == EINTR) {
          continue;
        }
      }

      for (auto* cqe = nextCompletion(); cqe != nullptr; cqe = nextCompletion()) {
        // Handle timeout
        auto* handler = io_uring_cqe_get_data(cqe);
        static_cast<CompletionHandler*>(handler)->handle(cqe);
        io_uring_cqe_seen(&ring_, cqe);
      }
      executeTasks();
    }
    executeTasks();
  }

  auto schedule() {
    return stdexec::__make_sexpr<ScheduleT>(this);
  }

  auto scheduleAfter(std::chrono::steady_clock::duration duration) {
    return stdexec::__make_sexpr<TimedScheduleT>(stdexec::__tup::__mktuple(this, duration));
  }

  struct TaskBase {
    virtual ~TaskBase() = default;

    virtual void run() noexcept = 0;
    virtual void requestStop() noexcept = 0;
  };

  template <typename Receiver>
  struct Task : TaskBase {
    Context* self;
    Receiver receiver;

    Task(Context* self, Receiver& receiver) : self(self), receiver(receiver) {};

    void requestStop() noexcept override {
      stdexec::set_stopped(receiver);
    }

    void start() noexcept {
      self->enqueue(this);
    }

    void run() noexcept override {
      stdexec::set_value(receiver);
    }
  };

  template <typename Receiver>
  struct TimedTask : TimedTaskBase {
    Ticker* ticker;
    Task<Receiver> task;
    std::chrono::steady_clock::duration duration;

    TimedTask(Ticker* ticker, Context* self, Receiver& receiver, std::chrono::steady_clock::duration duration)
      : ticker(ticker), task(self, receiver), duration(duration) {};

    void start() noexcept {
      ticker->tickAfter(this, duration);
    }

    void tick() noexcept override {
      task.start();
    }

    void requestStop() noexcept override {
      task.requestStop();
    }
  };

  template <typename Receiver>
  auto createTask(Receiver& receiver) {
    return Task<Receiver>{ this, receiver };
  }

  template <typename Receiver>
  auto createTimedTask(Receiver& receiver, std::chrono::steady_clock::duration duration) {
    return TimedTask<Receiver>{ &ticker_, this, receiver, duration };
  }

  void requestStop() {
    stop_source_.request_stop();
    ticker_.requestStop();
    running_.store(false, std::memory_order_release);
  }

  [[nodiscard]] auto isRunning() const -> bool {
    return running_.load(std::memory_order_acquire);
  }

  auto stopToken() {
    return stop_source_.get_token();
  }

private:
  io_uring ring_;
  stdexec::inplace_stop_source stop_source_;
  Ticker ticker_;
  // TODO: switch to intrusive list...
  std::vector<TaskBase*> tasks_;

  // TODO: wakeup from submission...
  std::atomic<bool> running_{ true };

  auto getSqe() -> io_uring_sqe* {
    while (true) {
      io_uring_sqe* sqe{ nullptr };
      sqe = io_uring_get_sqe(&ring_);
      if (sqe == nullptr) {
        io_uring_submit(&ring_);
        continue;
      }
      return sqe;
    }
  }

  auto nextCompletion() -> io_uring_cqe* {
    unsigned head{ 0 };
    io_uring_cqe* cqe{ nullptr };
    io_uring_for_each_cqe(&ring_, head, cqe) {
      return cqe;
    }
    return nullptr;
  }

  void enqueue(TaskBase* task) {
    // TODO: thread handling
    tasks_.push_back(task);
  }

  void executeTasks() {
    while (!tasks_.empty()) {
      std::vector<TaskBase*> tasks;
      std::swap(tasks, tasks_);
      for (auto* task : tasks) {
        if (isRunning()) {
          task->run();
        } else {
          task->requestStop();
        }
      }
    }
  }

  friend struct CompletionHandler;
};
}  // namespace heph::conduit
