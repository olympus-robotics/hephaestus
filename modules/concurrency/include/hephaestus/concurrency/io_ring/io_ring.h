//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <system_error>

#include <absl/synchronization/mutex.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"
#include "hephaestus/containers/intrusive_fifo_queue.h"
#include "hephaestus/error_handling/panic.h"

namespace heph::concurrency::io_ring {

struct IoRingConfig {
  static constexpr std::uint32_t DEFAULT_ENTRY_COUNT = 8;
  std::uint32_t nentries{ DEFAULT_ENTRY_COUNT };
  std::uint32_t flags{ 0 };
};

class IoRing {
public:
  explicit IoRing(const IoRingConfig& config);

  void requestStop();

  void submit(IoRingOperationBase* operation);
  void runOnce(bool block = true);
  void run(
      const std::function<void()>& on_start = [] {},
      const std::function<bool()>& on_progress = [] { return false; });

  auto isRunning() const -> bool;
  auto isCurrent() const -> bool;
  auto hasWork() const -> bool;

  void notify(bool always = false) const;

private:
  auto getSqe() -> ::io_uring_sqe*;
  auto nextCompletion() -> io_uring_cqe*;

  struct NotifyOperation : IoRingOperationBase {
    IoRing* self;
    std::uint64_t dummy{ 0 };

    void prepare(::io_uring_sqe* sqe) final {
      ::io_uring_prep_read(sqe, self->notify_fd_, &dummy, sizeof(dummy), 0);
    }

    void handleCompletion(::io_uring_cqe* cqe) final {
      heph::panicIf(cqe->res != sizeof(dummy), "NotifyOperation: {}",
                    std::error_code(-cqe->res, std::system_category()).message());
      self->submit(this);
    }
  };

private:
  ::io_uring ring_{};
  NotifyOperation notify_operation_;
  int notify_fd_;
  IoRingConfig config_;
  std::atomic<bool> running_{ false };

  std::atomic<std::size_t> in_flight_{ 0 };
  absl::Mutex mutex_;
  containers::IntrusiveFifoQueue<IoRingOperationBase> outstanding_operations_;
  containers::IntrusiveFifoQueue<IoRingOperationBase> in_flight_operations_;
  static thread_local IoRing* current_ring;
};

}  // namespace heph::concurrency::io_ring
