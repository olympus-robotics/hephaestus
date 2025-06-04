
#include "hephaestus/concurrency/io_ring/io_ring.h"

#include <atomic>
#include <cerrno>
#include <functional>
#include <system_error>

#include <fmt/format.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/io_ring_operation_pointer.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::io_ring {
thread_local IoRing* IoRing::current_ring = nullptr;

struct DispatchOperation {
  static auto ring() -> IoRing& {
    static thread_local IoRing r{ { .nentries = IoRingConfig::DEFAULT_ENTRY_COUNT, .flags = 0 } };
    return r;
  }

  void prepare(::io_uring_sqe* sqe) {
    const IoRingOperationPointer self{ this };
    ::io_uring_prep_msg_ring(sqe, destination->ring_.ring_fd, 0, self.data, 0);
  }

  void handleCompletion(io_uring_cqe* cqe) {
    if (destination->isCurrentRing()) {
      // Resubmit operation to the ring.
      // Some operations don't need an extra prepare and simply act as a trigger
      // so we can omit the submit phase entirely
      if (operation.hasPrepare()) {
        destination->submit(operation);
      } else {
        operation.handleCompletion(nullptr);
      }
      submit_done.store(true, std::memory_order_release);
      submit_done.notify_all();
      return;
    }
    if (cqe->res < 0) {
      heph::panic(
          fmt::format("dispatch failed: {}", std::error_code(-cqe->res, std::system_category()).message()));
    }

    dispatch_done.store(true, std::memory_order_release);
  }

  void run() {
    destination->in_flight_.fetch_add(1, std::memory_order_release);
    ring().submit(*this);
    while (!dispatch_done.load(std::memory_order_acquire)) {
      ring().runOnce();
    }

    while (!submit_done.load(std::memory_order_acquire)) {
      submit_done.wait(false, std::memory_order_acquire);
    }
  }

  IoRing* destination{ nullptr };
  IoRingOperationPointer operation;
  std::atomic<bool> dispatch_done{ false };
  std::atomic<bool> submit_done{ false };
};

IoRing::IoRing(IoRingConfig const& config) : config_(config) {
  const int res = ::io_uring_queue_init(config_.nentries, &ring_, config_.flags);

  if (res < 0) {
    heph::panic(fmt::format("::io_uring_queue_init failed: {}",
                            std::error_code(-res, std::system_category()).message()));
  }
}

struct StopOperation {
  void handleCompletion() {
    self->requestStop();
    done.store(true, std::memory_order_release);
    done.notify_all();
  }

  void wait() {
    while (!done.load(std::memory_order_acquire)) {
      done.wait(false, std::memory_order_acquire);
    }
  }

  IoRing* self{ nullptr };
  std::atomic<bool> done{ false };
};

auto IoRing::stopRequested() -> bool {
  return stop_source_.stop_requested();
}

void IoRing::requestStop() {
  if (!isCurrentRing() && isRunning()) {
    StopOperation stop_operation{ .self = this, .done = false };
    DispatchOperation dispatch{ .destination = this,
                                .operation = IoRingOperationPointer{ &stop_operation },
                                .dispatch_done = false,
                                .submit_done = false };
    dispatch.run();
    stop_operation.wait();
    return;
  }
  stop_source_.request_stop();
}
auto IoRing::getStopToken() -> stdexec::inplace_stop_token {
  return stop_source_.get_token();
}

void IoRing::runOnce(bool block) {
  int res{ 0 };
  if (block) {
    res = ::io_uring_submit_and_wait(&ring_, 1);
  } else {
    res = ::io_uring_submit_and_get_events(&ring_);
  }
  if (res < 0 && !(res == -EAGAIN || res == -EINTR)) {
    heph::panic(fmt::format("::io_uring_submit_and_wait failed: {}",
                            std::error_code(-res, std::system_category()).message()));
  }

  for (auto* cqe = nextCompletion(); cqe != nullptr; cqe = nextCompletion()) {
    const IoRingOperationPointer operation{ io_uring_cqe_get_data64(cqe) };
    operation.handleCompletion(cqe);
    io_uring_cqe_seen(&ring_, cqe);
    if ((cqe->flags & IORING_CQE_F_MORE) == 0) {
      in_flight_.fetch_sub(1, std::memory_order_release);
    }
  }
}

void IoRing::run(const std::function<void()>& on_started, const std::function<bool()>& on_progress) {
  if (current_ring != nullptr) {
    heph::panic("Cannot run ring, another ring is already active for this thread");
  }

  int res = 0;

  res = ::io_uring_register_ring_fd(&ring_);

  if (res < 0) {
    heph::panic(fmt::format("::io_uring_register_ring_fd failed: {}",
                            std::error_code(-res, std::system_category()).message()));
  }
  current_ring = this;
  running_.store(true, std::memory_order_release);
  on_started();
  bool more_work = on_progress();
  while (more_work || !stop_source_.stop_requested() || in_flight_.load(std::memory_order_acquire) > 0) {
    runOnce(!more_work);
    more_work = on_progress();
  }
  res = ::io_uring_unregister_ring_fd(&ring_);

  if (res < 0) {
    heph::panic(fmt::format("::io_uring_unregister_ring_fd failed: {}",
                            std::error_code(-res, std::system_category()).message()));
  }
  current_ring = nullptr;
}

auto IoRing::isCurrentRing() -> bool {
  return current_ring == this;
}

auto IoRing::isRunning() -> bool {
  return running_.load(std::memory_order_acquire);
}

void IoRing::submit(IoRingOperationPointer operation) {
  // We need to dispatch to our ring if we are calling this function from outside
  // the event loop
  if (!isCurrentRing() && isRunning()) {
    DispatchOperation dispatch{
      .destination = this, .operation = operation, .dispatch_done = false, .submit_done = false
    };
    dispatch.run();
    return;
  }
  auto* sqe = getSqe();

  operation.prepare(sqe);

  ::io_uring_sqe_set_data64(sqe, operation.data);
}

auto IoRing::getSqe() -> ::io_uring_sqe* {
  while (!stop_source_.stop_requested() || in_flight_.load(std::memory_order_acquire) > 0) {
    if (::io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_); sqe != nullptr) {
      in_flight_.fetch_add(1, std::memory_order_release);
      return sqe;
    }
    const int res = ::io_uring_submit(&ring_);
    if (res < 0 && !(-res == EAGAIN || -res == EINTR)) {
      heph::panic(fmt::format("::io_uring_submit failed: {}",
                              std::error_code(-res, std::system_category()).message()));
    }
  }
  return nullptr;
}

auto IoRing::nextCompletion() -> io_uring_cqe* {
  unsigned head{ 0 };
  io_uring_cqe* cqe{ nullptr };
  io_uring_for_each_cqe(&ring_, head, cqe) {
    return cqe;
  }
  return nullptr;
}
}  // namespace heph::concurrency::io_ring
