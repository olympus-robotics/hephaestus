//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <optional>
#include <system_error>
#include <type_traits>

#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"
#include "hephaestus/error_handling/panic.h"

namespace heph::concurrency::io_ring {

template <typename IoRingOperationT>
struct StoppableIoRingOperation : IoRingOperationBase {
  struct StopOperation : IoRingOperationBase {
    void prepare(::io_uring_sqe* sqe) final;

    void handleCompletion(::io_uring_cqe* cqe) final;

    StoppableIoRingOperation* self;
  };

  struct StopCallback {
    StoppableIoRingOperation* self;
    void operator()() const noexcept {
      self->requestStop();
    }
  };
  using StopCallbackT = stdexec::stop_callback_for_t<stdexec::inplace_stop_token, StopCallback>;

  StoppableIoRingOperation(IoRingOperationT op, IoRing& ring);

  void submit(stdexec::inplace_stop_token token);
  template <typename Token>
  void submit(Token /*token*/) {
    submit();
  }
  void submit();

  void prepare(::io_uring_sqe* sqe) final;

  void handleCompletion(::io_uring_cqe* cqe) final;

  void requestStop();

  IoRingOperationT operation;
  IoRing* ring{ nullptr };
  absl::Mutex mutex;
  std::size_t in_flight{ 1 };
  bool stop_requested{ false };
  std::optional<StopOperation> stop_operation;
  std::optional<stdexec::inplace_stop_token> stop_token;
  std::optional<StopCallbackT> stop_callback;
};
template <typename IoRingOperationT>
inline StoppableIoRingOperation<IoRingOperationT>::StoppableIoRingOperation(IoRingOperationT op,
                                                                            IoRing& io_ring)
  : operation(std::move(op)), ring(&io_ring) {
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::StopOperation::prepare(::io_uring_sqe* sqe) {
  ::io_uring_prep_cancel(sqe, self, IORING_ASYNC_CANCEL_ALL);
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::StopOperation::handleCompletion(::io_uring_cqe* cqe) {
  bool stop = false;
  {
    absl::MutexLock l{ &self->mutex };
    // fmt::println(stderr, "{} StopOperation::handleCompletion {}", fmt::ptr(self), self->in_flight);
    --self->in_flight;
    if (self->in_flight == 0) {
      stop = true;
      self->stop_callback.reset();
    }
    const int res = cqe->res;
    if (res < 0) {
      if (res != -ENOENT && res != -EALREADY) {
        panic("StopOperation failed: {}", std::error_code(-res, std::system_category()).message());
      }
    }
  }
  if (stop) {
    self->operation.handleStopped();
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::submit(stdexec::inplace_stop_token token) {
  stop_token.emplace(token);
  submit();
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::submit() {
  ring->submit(this);
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::prepare(::io_uring_sqe* sqe) {
  if (stop_token.has_value()) {
    auto token = *stop_token;
    stop_callback.emplace(*stop_token, StopCallback{ this });
  }
  {
    absl::MutexLock l{ &mutex };
    // fmt::println(stderr, "{} prepare {}", fmt::ptr(this), in_flight);
    if (stop_requested) {
      io_uring_prep_nop(sqe);
      return;
    }
  }
  operation.prepare(sqe);
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::handleCompletion(::io_uring_cqe* cqe) {
  std::optional<absl::MutexLock> l{ &mutex };
  /// fmt::println(stderr, "{} handleCompletion {}", fmt::ptr(this), in_flight);
  // stop_callback.reset();
  --in_flight;
  if (cqe->res == -ECANCELED || stop_requested) {
    // fmt::println(stderr, "{} handleCompletion stop {}", fmt::ptr(this), in_flight);
    if (in_flight == 0) {
      l.reset();
      operation.handleStopped();
    }
  } else {
    using CompletionReturnT = decltype(operation.handleCompletion(cqe));
    if constexpr (std::is_same_v<CompletionReturnT, bool>) {
      l.reset();
      auto tmp = in_flight;
      if (!operation.handleCompletion(cqe)) {
        {
          absl::MutexLock l1{ &mutex };
          ++in_flight;
        }
        // fmt::println(stderr, "{} handleCompletion resubmit {}", fmt::ptr(this), in_flight);
        ring->submit(this);
        return;
      }
      // fmt::println(stderr, "{} handleCompletion done(bool) {}", fmt::ptr(this), tmp);
      return;
    }
    // fmt::println(stderr, "{} handleCompletion done {}", fmt::ptr(this), in_flight);
    l.reset();
    operation.handleCompletion(cqe);
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::requestStop() {
  {
    absl::MutexLock l{ &mutex };
    // fmt::println(stderr, "{} requestStop {}", fmt::ptr(this), in_flight);
    if (in_flight == 0) {
      return;
    }
    assert(!stop_operation.has_value());
    stop_operation.emplace();
    stop_operation->self = this;
    ++in_flight;
    stop_requested = true;
  }
  ring->submit(&stop_operation.value());
}

}  // namespace heph::concurrency::io_ring
