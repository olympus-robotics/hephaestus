//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <liburing.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring_operation_pointer.h"

namespace heph::concurrency {

struct IoRingConfig {
  static constexpr unsigned DEFAULT_NENTRIES = 1024;
  unsigned nentries{ DEFAULT_NENTRIES };
  unsigned flags{ IORING_SETUP_DEFER_TASKRUN | IORING_SETUP_SINGLE_ISSUER };
};

class IoRing {
public:
  explicit IoRing(IoRingConfig const& config);

  void requestStop();
  auto getStopToken() -> stdexec::inplace_stop_token;

  template <typename IoRingOperationT>
  void submit(IoRingOperationT& operation);
  void runOnce();
  void run(std::function<void()> on_start = [] {}, std::function<void()> on_progress = [] {});

  auto isRunning() -> bool;
  auto isCurrentRing() -> bool;

private:
  ::io_uring ring_{};
  std::atomic<bool> running_{ false };
  stdexec::inplace_stop_source stop_source_;

  std::atomic<std::size_t> in_flight_{ 0 };

  void submit(IoRingOperationPointer operation);
  auto getSqe() -> ::io_uring_sqe*;
  auto nextCompletion() -> io_uring_cqe*;

  static thread_local IoRing* current_ring;

  friend struct DispatchOperation;
  friend struct StopOperation;
};

template <typename IoRingOperationT>
void IoRing::submit(IoRingOperationT& operation) {
  submit(IoRingOperationPointer{ &operation });
}

}  // namespace heph::concurrency
