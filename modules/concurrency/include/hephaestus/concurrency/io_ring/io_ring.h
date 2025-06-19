//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>

#include <liburing.h>
#include <liburing/io_uring.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/io_ring_operation_base.h"

namespace heph::concurrency::io_ring {

struct IoRingConfig {
  static constexpr std::uint32_t DEFAULT_ENTRY_COUNT = 1024;
  std::uint32_t nentries{ DEFAULT_ENTRY_COUNT };
  std::uint32_t flags{ 0 };
};

class IoRing {
public:
  explicit IoRing(const IoRingConfig& config);

  auto stopRequested() -> bool;
  void requestStop();
  auto getStopToken() -> stdexec::inplace_stop_token;

  void submit(IoRingOperationBase* operation);
  void runOnce(bool block = true);
  void run(
      const std::function<void()>& on_start = [] {},
      const std::function<bool()>& on_progress = [] { return false; });

  auto isRunning() -> bool;
  auto isCurrentRing() -> bool;

private:
  auto getSqe() -> ::io_uring_sqe*;
  auto nextCompletion() -> io_uring_cqe*;

  friend struct DispatchOperation;
  friend struct StopOperation;

private:
  ::io_uring ring_{};
  IoRingConfig config_;
  std::atomic<bool> running_{ false };
  stdexec::inplace_stop_source stop_source_;

  std::atomic<std::size_t> in_flight_{ 0 };
  static thread_local IoRing* current_ring;
};

}  // namespace heph::concurrency::io_ring
