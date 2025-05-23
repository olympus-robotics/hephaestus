//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/detail/node_base.h"

#include <chrono>

#include <stdexec/stop_token.hpp>

#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"

namespace heph::conduit::detail {

void NodeBase::updateExecutionTime(std::chrono::nanoseconds duration) {
  last_execution_duration_ = duration;
}

auto NodeBase::lastPeriodDuration() -> std::chrono::nanoseconds {
  auto period = nodePeriod();
  std::chrono::nanoseconds start_after = period - last_execution_duration_;
  // We attempt to avoid drift by adapting the expiry time with the
  // measured execution time of the last execution duration.
  // If execution took longer, we should scheduler immediately (delay of 0).
  if (last_execution_duration_ >= period) {
    heph::log(heph::WARN, std::string{ MISSED_DEADLINE_WARNING }, "node", nodeName(), "period", period,
              "duration", last_execution_duration_);
    start_after = std::chrono::nanoseconds{ 0 };
  }
  return start_after;
};

ExecutionStopWatch::~ExecutionStopWatch() noexcept {
  auto end = std::chrono::high_resolution_clock::now();
  self_->updateExecutionTime(end - start_);
}

ExecutionStopWatch::ExecutionStopWatch(NodeBase* self)
  : self_(self), start_(std::chrono::high_resolution_clock::now()) {
}
auto NodeBase::getStopToken() -> stdexec::inplace_stop_token {
  if (engine_ == nullptr) {
    return stdexec::inplace_stop_token{};
  }
  return engine_->getStopToken();
}
}  // namespace heph::conduit::detail
