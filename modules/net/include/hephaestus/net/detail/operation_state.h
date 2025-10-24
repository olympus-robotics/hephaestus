//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <optional>

#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/io_ring/io_ring.h"
#include "hephaestus/concurrency/io_ring/stoppable_io_ring_operation.h"

namespace heph::net::detail {

template <typename Operation>
class OperationState {
public:
  OperationState(concurrency::io_ring::IoRing* io_ring, Operation&& operation)
    : operation_(std::move(operation), *io_ring) {
  }

  void submit() {
    operation_.submit(operation_.operation.getStopToken());
  }

private:
  concurrency::io_ring::StoppableIoRingOperation<Operation> operation_;
};

}  // namespace heph::net::detail
