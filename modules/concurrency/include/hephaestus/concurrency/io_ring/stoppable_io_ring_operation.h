//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cerrno>
#include <optional>
#include <system_error>
#include <type_traits>

#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::io_ring {

template <typename IoRingOperationT>
struct StoppableIoRingOperation : IoRingOperationBase {
  struct StopCallback {
    StoppableIoRingOperation* self;

    void operator()() const {
      self->requestStop();
    }
  };
  struct StopOperation : IoRingOperationBase {
    void prepare(::io_uring_sqe* sqe) final;

    void handleCompletion(::io_uring_cqe* cqe) final;

    StoppableIoRingOperation* self;
  };

  StoppableIoRingOperation(IoRingOperationT op, IoRing& ring, stdexec::inplace_stop_token token);

  void prepare(::io_uring_sqe* sqe) final;

  void handleCompletion(::io_uring_cqe* cqe) final;

  void requestStop();

  IoRingOperationT operation;
  IoRing* ring{ nullptr };
  stdexec::inplace_stop_callback<StopCallback> stop_callback;
  int in_flight{ 1 };
  std::optional<StopOperation> stop_operation;
};
template <typename IoRingOperationT>
inline StoppableIoRingOperation<IoRingOperationT>::StoppableIoRingOperation(IoRingOperationT op,
                                                                            IoRing& io_ring,
                                                                            stdexec::inplace_stop_token token)
  : operation(std::move(op)), ring(&io_ring), stop_callback(token, StopCallback{ this }) {
  if (token.stop_requested()) {
    in_flight = 0;
    operation.handleStopped();
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::StopOperation::prepare(::io_uring_sqe* sqe) {
  ::io_uring_prep_cancel(sqe, self, IORING_ASYNC_CANCEL_ALL);
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::StopOperation::handleCompletion(::io_uring_cqe* cqe) {
  --self->in_flight;
  const int res = cqe->res;
  if (res < 0) {
    if (res != -ENOENT && res != -EALREADY) {
      panic("StopOperation failed: {}", std::error_code(-res, std::system_category()).message());
    }
  }
  if (self->in_flight == 0) {
    self->operation.handleStopped();
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::prepare(::io_uring_sqe* sqe) {
  if (stop_operation.has_value()) {
    io_uring_prep_nop(sqe);
    return;
  }
  operation.prepare(sqe);
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::handleCompletion(::io_uring_cqe* cqe) {
  --in_flight;
  if (cqe->res == -ECANCELED || stop_operation.has_value()) {
    if (in_flight == 0) {
      operation.handleStopped();
    }
    return;
  }
  using CompletionReturnT = decltype(operation.handleCompletion(cqe));
  if constexpr (std::is_same_v<CompletionReturnT, bool>) {
    if (!operation.handleCompletion(cqe)) {
      ++in_flight;
      ring->submit(this);
    }
  } else {
    operation.handleCompletion(cqe);
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::requestStop() {
  if (in_flight == 0) {
    return;
  }
  stop_operation.emplace();
  stop_operation->self = this;
  ++in_flight;
  ring->submit(&stop_operation.value());
}

}  // namespace heph::concurrency::io_ring
