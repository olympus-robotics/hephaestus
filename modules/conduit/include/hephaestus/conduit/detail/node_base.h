//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

#include <hephaestus/concurrency/context.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/io_ring/timer.h"
#include "hephaestus/conduit/detail/output_connections.h"

// Forward declarations
namespace heph::conduit {
class NodeEngine;
}

namespace heph::conduit::detail {

class NodeBase {
public:
  using ClockT = concurrency::io_ring::TimerClock;

  static constexpr std::string_view MISSED_DEADLINE_WARNING = "Missed deadline";

  virtual ~NodeBase() = default;
  [[nodiscard]] virtual auto nodeName() const -> std::string = 0;
  [[nodiscard]] virtual auto nodePeriod() -> std::chrono::nanoseconds = 0;
  virtual void removeOutputConnection(void* node) = 0;

  auto engine() -> NodeEngine& {
    return *engine_;
  }

  [[nodiscard]] auto engine() const -> const NodeEngine& {
    return *engine_;
  }
  auto enginePtr() -> NodeEngine* {
    return engine_;
  }

  [[nodiscard]] auto enginePtr() const -> const NodeEngine* {
    return engine_;
  }

  [[nodiscard]] auto runsOnEngine() const -> bool;

  [[nodiscard]] auto scheduler() const -> concurrency::Context::Scheduler;

  auto getStopToken() -> stdexec::inplace_stop_token;

protected:
  auto operationStart(bool has_period) -> ClockT::time_point;
  void operationEnd();
  void updateExecutionTime(std::chrono::nanoseconds duration);
  auto nextStartTime(bool has_period) -> ClockT::time_point;

private:
  friend class heph::conduit::NodeEngine;
  friend class ExecutionStopWatch;
  NodeEngine* engine_{ nullptr };
  std::chrono::nanoseconds last_execution_duration_{};

  ClockT::time_point last_steady_;
  std::chrono::system_clock::time_point last_system_;
  ClockT::time_point start_time_;
  std::size_t iteration_{ 0 };
};

class ExecutionStopWatch {
public:
  explicit ExecutionStopWatch(NodeBase* self);
  ~ExecutionStopWatch() noexcept;

  ExecutionStopWatch(const ExecutionStopWatch&) = delete;
  auto operator=(const ExecutionStopWatch&) -> ExecutionStopWatch& = delete;
  ExecutionStopWatch(ExecutionStopWatch&&) = delete;
  auto operator=(ExecutionStopWatch&&) -> ExecutionStopWatch& = delete;

private:
  NodeBase* self_;
  std::chrono::high_resolution_clock::time_point start_;
};
}  // namespace heph::conduit::detail
