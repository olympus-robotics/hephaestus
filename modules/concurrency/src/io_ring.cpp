
#include "hephaestus/concurrency/io_ring.h"

#include <atomic>
#include <cerrno>
#include <system_error>

#include <fmt/format.h>
#include <hephaestus/concurrency/io_ring_operation_pointer.h>

#include "hephaestus/utils/exception.h"

namespace heph::concurrency {
thread_local IoRing* IoRing::current_ring = nullptr;

struct DispatchOperation {
  static auto ring() -> IoRing& {
    static thread_local IoRing r{ {} };
    return r;
  }

  void prepare(::io_uring_sqe* sqe) const {
    ::io_uring_prep_msg_ring(sqe, destination->ring_.ring_fd, 0, operation.data, 0);
  }

  void handleCompletion(io_uring_cqe* cqe) {
    heph::panicIf(cqe->res < 0, fmt::format("dispatch failed: {}",
                                            std::error_code(-cqe->res, std::system_category()).message()));

    done.store(true, std::memory_order_release);
  }

  void requestStop() {
  }

  void run() {
    destination->in_flight_.fetch_add(1, std::memory_order_release);
    ring().submit(*this);
    while (!done.load(std::memory_order_acquire)) {
      ring().runOnce();
    }
  }

  IoRing* destination{ nullptr };
  IoRingOperationPointer operation;
  std::atomic<bool> done{ false };
};

IoRing::IoRing(IoRingConfig const& config) {
  int res = ::io_uring_queue_init(config.nentries, &ring_, config.flags);

  heph::panicIf(res < 0, fmt::format("::io_uring_queue_init failed: {}",
                                     std::error_code(-res, std::system_category()).message()));

  res = ::io_uring_register_ring_fd(&ring_);

  heph::panicIf(res < 0, fmt::format("::io_uring_register_ring_fd failed: {}",
                                     std::error_code(-res, std::system_category()).message()));
}

struct StopOperation {
  void prepare(::io_uring_sqe* /*sqe*/) const {
    heph::panic("StopOperation::prepare should not be called");
  }

  void handleCompletion(io_uring_cqe* cqe) {
    heph::panicIf(cqe->res < 0, fmt::format("stop failed: {}",
                                            std::error_code(-cqe->res, std::system_category()).message()));

    self->requestStop();
    done.store(true, std::memory_order_release);
    done.notify_all();
  }

  void requestStop() {
  }

  void wait() {
    while (!done.load(std::memory_order_acquire)) {
      done.wait(false, std::memory_order_acquire);
    }
  }

  IoRing* self{ nullptr };
  std::atomic<bool> done{ false };
};

void IoRing::requestStop() {
  if (current_ring != this && running_.load(std::memory_order_acquire)) {
    StopOperation stop_operation{ this, false };
    DispatchOperation dispatch{ this, IoRingOperationPointer{ &stop_operation }, false };
    dispatch.run();
    stop_operation.wait();
    return;
  }
  stop_source_.request_stop();
}
auto IoRing::getStopToken() -> stdexec::inplace_stop_token {
  return stop_source_.get_token();
}

void IoRing::runOnce() {
  int res = ::io_uring_submit_and_wait(&ring_, 1);
  heph::panicIf(res < 0 && !(res == -EAGAIN || res == -EINTR),
                fmt::format("::io_uring_submit_and_wait failed: {}",
                            std::error_code(-res, std::system_category()).message()));

  for (auto* cqe = nextCompletion(); cqe != nullptr; cqe = nextCompletion()) {
    IoRingOperationPointer operation{ io_uring_cqe_get_data64(cqe) };
    operation.handleCompletion(cqe);
    io_uring_cqe_seen(&ring_, cqe);
    in_flight_.fetch_sub(1, std::memory_order_release);
  }
}

void IoRing::run(std::function<void()> on_started) {
  heph::panicIf(current_ring != nullptr, "Cannot run ring, another ring is already active for this thread");
  current_ring = this;
  running_.store(true, std::memory_order_release);
  on_started();
  while (!stop_source_.stop_requested() || in_flight_.load(std::memory_order_acquire) > 0) {
    runOnce();
  }
  current_ring = nullptr;
}

void IoRing::submit(IoRingOperationPointer operation) {
  // We need to dispatch to our ring if we are calling this function from outside
  // the event loop
  if (current_ring != this && running_.load(std::memory_order_acquire)) {
    DispatchOperation dispatch{ this, operation, false };
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
    int res = ::io_uring_submit(&ring_);
    if (res < 0 && !(-res == EAGAIN || -res == EINTR)) {
      heph::panicIf(res < 0, fmt::format("::io_uring_submit failed: {}",
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
}  // namespace heph::concurrency
