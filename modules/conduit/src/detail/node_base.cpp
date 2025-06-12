//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/detail/node_base.h"

#include <chrono>

#include <fmt/format.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/metric_record.h"
#include "hephaestus/telemetry/metric_sink.h"

namespace heph::conduit::detail {

auto NodeBase::operationStart(bool has_period) -> ClockT::time_point {
  auto start_at = nextStartTime(has_period);
  last_steady_ = ClockT::now();
  last_system_ = std::chrono::system_clock::now();
  if (iteration_ == 0) {
    start_time_ = last_steady_;
  }
  return start_at;
}

void NodeBase::operationEnd() {
  auto now = ClockT::now();
  auto system_now = std::chrono::system_clock::now();

  auto tick_duration = now - last_steady_;
  auto clock_drift =
      std::chrono::duration_cast<std::chrono::microseconds>((system_now - last_system_) - tick_duration);

  std::chrono::microseconds period_drift{ 0 };
  auto period = nodePeriod();
  if (period != std::chrono::nanoseconds{ 0 }) {
    period_drift =
        std::chrono::duration_cast<std::chrono::microseconds>(now - (start_time_ + (iteration_ * period)));
  }

  if (iteration_ > 0) {
    heph::telemetry::record([name = nodeName(), timestamp = std::chrono::system_clock::now(),
                             last_execution_duration = last_execution_duration_, tick_duration, clock_drift,
                             period_drift] {
      return heph::telemetry::Metric{
        .component = fmt::format("conduit/{}", name),
        .tag = "node_timings",
        .timestamp = timestamp,
        .values = { { "execute_duration_microsec",
                      std::chrono::duration_cast<std::chrono::microseconds>(last_execution_duration)
                          .count() },
                    { "tick_duration_microsec",
                      std::chrono::duration_cast<std::chrono::microseconds>(tick_duration).count() },
                    { "clock_drift_microsec", clock_drift.count() },
                    { "period_drift_microsec", period_drift.count() } }
      };
    });
  }
  ++iteration_;
}

void NodeBase::updateExecutionTime(std::chrono::nanoseconds duration) {
  last_execution_duration_ = duration;
}

auto NodeBase::nextStartTime(bool has_period) -> ClockT::time_point {
  auto now = ClockT::now();
  if (last_execution_duration_ == std::chrono::nanoseconds{ 0 } || !has_period) {
    return now;
  }

  auto period = nodePeriod();
  auto next_time_point = start_time_ + (iteration_ * period);
  if (next_time_point < now) {
    auto last_duration = now - last_steady_;
    heph::log(heph::WARN, std::string{ MISSED_DEADLINE_WARNING }, "node", nodeName(), "period", period,
              "tick_duration", last_duration, "execution_duration", last_execution_duration_);
    return now;
  }
  return next_time_point;
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
auto NodeBase::runsOnEngine() const -> bool {
  if (engine_ == nullptr) {
    return true;
  }
  return engine_->isCurrent();
}
}  // namespace heph::conduit::detail
