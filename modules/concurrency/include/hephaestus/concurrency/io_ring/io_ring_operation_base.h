//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

namespace heph::concurrency::io_ring {

struct IoRingOperationBase {
  virtual ~IoRingOperationBase() = default;

  virtual void prepare(::io_uring_sqe* sqe) {
    ::io_uring_prep_nop(sqe);
  }
  virtual void handleCompletion(::io_uring_cqe* cqe) = 0;

  IoRingOperationBase* next{ nullptr };
  IoRingOperationBase* prev{ nullptr };
};
}  // namespace heph::concurrency::io_ring
