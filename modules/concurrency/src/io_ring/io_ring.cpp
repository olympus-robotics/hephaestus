
#include "hephaestus/concurrency/io_ring/io_ring.h"

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <system_error>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>
#include <sys/eventfd.h>

#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"
#include "hephaestus/error_handling/panic.h"

namespace heph::concurrency::io_ring {
thread_local IoRing* IoRing::current_ring = nullptr;

IoRing::IoRing(const IoRingConfig& config) : notify_fd_(::eventfd(0, 0)), config_(config) {
  const int res = ::io_uring_queue_init(config_.nentries, &ring_, config_.flags);

  if (res < 0) {
    panic("::io_uring_queue_init failed: {}", std::error_code(-res, std::system_category()).message());
  }
  notify_operation_.self = this;

  submit(&notify_operation_);
}

void IoRing::requestStop() {
  running_.store(false, std::memory_order_release);
  notify(true);
}

void IoRing::runOnce(bool block) {
  containers::IntrusiveFifoQueue<IoRingOperationBase> outstanding_operations;
  {
    absl::MutexLock l{ &mutex_ };
    std::swap(outstanding_operations, outstanding_operations_);
    assert(outstanding_operations_.empty());
  }

  while (true) {
    auto* operation = outstanding_operations.dequeue();
    if (operation == nullptr) {
      break;
    }
    auto* sqe = getSqe();

    operation->prepare(sqe);

    ::io_uring_sqe_set_data(sqe, operation);
  }

  int res{ 0 };
  if (block) {
    res = ::io_uring_submit_and_wait(&ring_, 1);
  } else {
    res = ::io_uring_submit_and_get_events(&ring_);
  }
  if (res < 0 && !(res == -EAGAIN || res == -EINTR)) {
    panic("::io_uring_submit_and_wait failed: {}", std::error_code(-res, std::system_category()).message());
  }

  for (auto* cqe = nextCompletion(); cqe != nullptr; cqe = nextCompletion()) {
    auto* operation{ static_cast<IoRingOperationBase*>(io_uring_cqe_get_data(cqe)) };
    operation->handleCompletion(cqe);
    io_uring_cqe_seen(&ring_, cqe);
    if ((cqe->flags & IORING_CQE_F_MORE) == 0) {
      in_flight_.fetch_sub(1, std::memory_order_release);
    }
  }
}

void IoRing::run(const std::function<void()>& on_started, const std::function<bool()>& on_progress) {
  if (current_ring != nullptr) {
    panic("Cannot run ring, another ring is already active for this thread");
  }

  int res = 0;
  res = ::io_uring_register_ring_fd(&ring_);

  if (res < 0) {
    panic("::io_uring_register_ring_fd failed: {}", std::error_code(-res, std::system_category()).message());
  }

  current_ring = this;
  bool running = false;
  running_.compare_exchange_strong(running, true, std::memory_order_acq_rel);

  on_started();
  bool more_work = on_progress();
  while (more_work || running_.load(std::memory_order_acquire) ||
         in_flight_.load(std::memory_order_acquire) > 1) {
    runOnce(!more_work);
    more_work = on_progress();
  }

  res = ::io_uring_unregister_ring_fd(&ring_);
  if (res < 0) {
    panic("::io_uring_unregister_ring_fd failed: {}",
          std::error_code(-res, std::system_category()).message());
  }
  current_ring = nullptr;
}

auto IoRing::isCurrent() const -> bool {
  return current_ring == this;
}

auto IoRing::isRunning() const -> bool {
  return running_.load(std::memory_order_acquire);
}

void IoRing::submit(IoRingOperationBase* operation) {
  {
    absl::MutexLock l{ &mutex_ };
    outstanding_operations_.enqueue(operation);
  }
  notify();
}

void IoRing::notify(bool always) const {
  if (always || (!isCurrent() && isRunning())) {
    std::uint64_t dummy{ 1 };
    auto res = ::write(notify_fd_, &dummy, sizeof(dummy));
    heph::panicIf(res != sizeof(dummy), "IoRing::notify: {}",
                  std::error_code{ errno, std::system_category() }.message());
  }
}

auto IoRing::getSqe() -> ::io_uring_sqe* {
  while (true) {
    if (::io_uring_sqe* sqe = ::io_uring_get_sqe(&ring_); sqe != nullptr) {
      in_flight_.fetch_add(1, std::memory_order_release);
      return sqe;
    }
    const int res = ::io_uring_submit(&ring_);
    if (res < 0 && !(-res == EAGAIN || -res == EINTR)) {
      panic("::io_uring_submit failed: {}", std::error_code(-res, std::system_category()).message());
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
