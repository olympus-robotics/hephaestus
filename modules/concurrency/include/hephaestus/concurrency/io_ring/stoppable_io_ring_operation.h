//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cerrno>
#include <optional>
#include <system_error>
#include <type_traits>

#include <fmt/format.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_pointer.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::io_ring {

template <typename IoRingOperationT>
struct StoppableIoRingOperation {
  struct StopCallback {
    StoppableIoRingOperation* self;

    void operator()() const {
      self->requestStop();
    }
  };
  struct StopOperation {
    void prepare(::io_uring_sqe* sqe);

    void handleCompletion(::io_uring_cqe* cqe);

    StoppableIoRingOperation* self;
  };

  StoppableIoRingOperation(IoRingOperationT op, IoRing& ring, stdexec::inplace_stop_token token);

  void prepare(::io_uring_sqe* sqe);

  void handleCompletion(::io_uring_cqe* cqe);

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
    operation.handleStopped();
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::StopOperation::prepare(::io_uring_sqe* sqe) {
  IoRingOperationPointer ptr{ self };
  ::io_uring_prep_cancel64(sqe, ptr.data, IORING_ASYNC_CANCEL_ALL);
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::StopOperation::handleCompletion(::io_uring_cqe* cqe) {
  --self->in_flight;
  const int res = cqe->res;
  if (res == -ENOENT || res == -EALREADY) {
    return;
  }
  if (res < 0) {
    heph::panic("StopOperation failed: {}", std::error_code(-res, std::system_category()).message());
  }
  if (self->in_flight == 0) {
    self->operation.handleStopped();
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::prepare(::io_uring_sqe* sqe) {
  if (stop_operation.has_value()) {
    return;
  }
  operation.prepare(sqe);
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::handleCompletion(::io_uring_cqe* cqe) {
  if (cqe->res == -ECANCELED || stop_operation.has_value()) {
    --in_flight;
    if (in_flight == 0) {
      operation.handleStopped();
    }
    return;
  }
  using CompletionReturnT = decltype(operation.handleCompletion(cqe));
  if constexpr (std::is_same_v<CompletionReturnT, bool>) {
    if (!operation.handleCompletion(cqe)) {
      ring->submit(*this);
    }
  } else {
    operation.handleCompletion(cqe);
  }
}

template <typename IoRingOperationT>
inline void StoppableIoRingOperation<IoRingOperationT>::requestStop() {
  ++in_flight;
  stop_operation.emplace(StopOperation{ this });
  ring->submit(stop_operation.value());
}

}  // namespace heph::concurrency::io_ring
