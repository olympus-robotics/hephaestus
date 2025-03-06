
#pragma once

#include <chrono>
#include <ctime>
#include <functional>
#include <queue>
#include <vector>

#include <liburing.h>

#include "hephaestus/conduit/completion_handler.h"

namespace heph::conduit {

struct TimedTaskBase {
  virtual ~TimedTaskBase() = default;
  virtual void tick() noexcept = 0;
  virtual void requestStop() noexcept = 0;
};

struct Ticker : CompletionHandler {
  explicit Ticker(Context* context, double time_scale_factor)
    : CompletionHandler{ context }, time_scale_factor(time_scale_factor) {
  }

  struct TickerEntry {
    TimedTaskBase* task{ nullptr };
    std::chrono::steady_clock::time_point timeout;

    friend auto operator<=>(TickerEntry const& lhs, TickerEntry const& rhs) {
      return lhs.timeout <=> rhs.timeout;
    }
  };

  void requestStop() {
    while (!queue.empty()) {
      const auto& top = queue.top();

      top.task->requestStop();

      queue.pop();
    }
  }

  void tickAfter(TimedTaskBase* task, std::chrono::steady_clock::duration duration) {
    auto task_timeout =
        std::chrono::steady_clock::now() +
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(duration * time_scale_factor);
    queue.emplace(task, task_timeout);

    auto const& top = queue.top();
    if (top.task != task) {
      return;
    }

    recharge(task_timeout);
  }

  void handle(io_uring_cqe* cqe) override {
    (void)cqe;
    for (TimedTaskBase* task = next(); task != nullptr; task = next()) {
      task->tick();
    }

    if (!queue.empty()) {
      auto const& top = queue.top();
      recharge(top.timeout);
    }
  }

  auto next() -> TimedTaskBase* {
    if (queue.empty()) {
      return nullptr;
    }
    auto const& top = queue.top();
    if (top.timeout <= std::chrono::steady_clock::now()) {
      auto* task = top.task;
      queue.pop();
      return task;
    }

    return nullptr;
  }

  void recharge(std::chrono::steady_clock::time_point time) {
    auto since_epoch = time.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
    timeout.tv_sec = seconds.count();
    timeout.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch - seconds).count();

    // TODO: update timeout instead of generating a new one each time...
    auto* sqe = getSqe();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    io_uring_prep_timeout(sqe, &timeout, 0, IORING_TIMEOUT_ABS);
    io_uring_sqe_set_data(sqe, this);
  }

  double time_scale_factor{ 1.0 };
  __kernel_timespec timeout{};
  std::priority_queue<TickerEntry, std::vector<TickerEntry>, std::greater<>> queue;
};
}  // namespace heph::conduit
