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
  struct StopCallback {
    OperationState* self;
    void operator()() const noexcept {
      self->stop_source_.request_stop();
    }
  };

  using EnvCallbackT = stdexec::stop_callback_for_t<typename Operation::StopTokenT, StopCallback>;
  using RingCallbackT = stdexec::stop_callback_for_t<stdexec::inplace_stop_token, StopCallback>;

  OperationState(concurrency::io_ring::IoRing* io_ring, Operation&& operation)
    : operation_(std::move(operation), *io_ring, stop_source_.get_token()) {
  }

  void submit() {
    auto* ring = operation_.ring;
    env_stop_.emplace(operation_.operation.getStopToken(), StopCallback{ this });
    ring_stop_.emplace(ring->getStopToken(), StopCallback{ this });
    ring->submit(operation_);
  }

private:
  stdexec::inplace_stop_source stop_source_;
  std::optional<EnvCallbackT> env_stop_;
  std::optional<RingCallbackT> ring_stop_;
  concurrency::io_ring::StoppableIoRingOperation<Operation> operation_;
};

}  // namespace heph::net::detail
