//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cerrno>
#include <system_error>

#include <fmt/format.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring.h"
#include "hephaestus/concurrency/io_ring_operation_pointer.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency {

template <typename IoRingOperationT>
struct StoppableIoRingOperation {
  struct StopCallback {
    StoppableIoRingOperation* self;

    void operator()() const {
      self->requestStop();
    }
  };
  struct StopOperation {
    void prepare(::io_uring_sqe* sqe) {
      IoRingOperationPointer ptr{ self };
      ::io_uring_prep_cancel64(sqe, ptr.data, IORING_ASYNC_CANCEL_ALL);
    }

    void handleCompletion(::io_uring_cqe* cqe) {
      --self->in_flight;
      const int res = cqe->res;
      if (res == -ENOENT || res == -EALREADY) {
        return;
      }
      heph::panicIf(res < 0, fmt::format("StopOperation failed: {}",
                                         std::error_code(-res, std::system_category()).message()));
      if (self->in_flight == 0) {
        self->operation.handleStopped();
      }
    }

    StoppableIoRingOperation* self;
  };

  explicit StoppableIoRingOperation(IoRingOperationT op, IoRing& ring, stdexec::inplace_stop_token token)
    : operation(std::move(op)), ring(&ring), stop_callback(token, StopCallback{ this }) {
    if (token.stop_requested()) {
      operation.handleStopped();
    }
  }

  void prepare(::io_uring_sqe* sqe) {
    if (stop_operation.has_value()) {
      return;
    }
    operation.prepare(sqe);
  }

  void handleCompletion(::io_uring_cqe* cqe) {
    if (cqe->res == -ECANCELED || stop_operation.has_value()) {
      --in_flight;
      if (in_flight == 0) {
        operation.handleStopped();
      }
      return;
    }
    operation.handleCompletion(cqe);
  }

  void requestStop() {
    stop_operation.emplace(StopOperation{ this });
    ring->submit(stop_operation.value());
  }

  IoRingOperationT operation;
  IoRing* ring{ nullptr };
  stdexec::inplace_stop_callback<StopCallback> stop_callback;
  int in_flight{ 1 };
  std::optional<StopOperation> stop_operation;
};
}  // namespace heph::concurrency
