//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include <stdexec/stop_token.hpp>

// Forward declarations
namespace heph::conduit {
class NodeEngine;
}

namespace heph::conduit::detail {

class NodeBase {
public:
  static constexpr std::string_view MISSED_DEADLINE_WARNING = "Missed deadline";

  virtual ~NodeBase() = default;
  [[nodiscard]] virtual auto nodeName() const -> std::string = 0;
  [[nodiscard]] virtual auto nodePeriod() -> std::chrono::nanoseconds = 0;

  auto engine() -> NodeEngine& {
    return *engine_;
  }

  [[nodiscard]] auto engine() const -> NodeEngine const& {
    return *engine_;
  }

  auto getStopToken() -> stdexec::inplace_stop_token;

protected:
  void updateExecutionTime(std::chrono::nanoseconds duration);
  auto lastPeriodDuration() -> std::chrono::nanoseconds;

private:
  friend class heph::conduit::NodeEngine;
  friend class ExecutionStopWatch;
  NodeEngine* engine_{ nullptr };
  std::chrono::nanoseconds last_execution_duration_{};
};

class ExecutionStopWatch {
public:
  explicit ExecutionStopWatch(NodeBase* self);
  ~ExecutionStopWatch() noexcept;

  ExecutionStopWatch(ExecutionStopWatch const&) = delete;
  auto operator=(ExecutionStopWatch const&) -> ExecutionStopWatch& = delete;
  ExecutionStopWatch(ExecutionStopWatch&&) = delete;
  auto operator=(ExecutionStopWatch&&) -> ExecutionStopWatch& = delete;

private:
  NodeBase* self_;
  std::chrono::high_resolution_clock::time_point start_;
};

}  // namespace heph::conduit::detail
