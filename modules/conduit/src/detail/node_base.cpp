//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#if 0
#include "hephaestus/conduit/detail/node_base.h"

#include <chrono>
#include <string>

#include <fmt/format.h>
#include <stdexec/stop_token.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/telemetry/log/log.h"
#include "hephaestus/telemetry/log/log_sink.h"
#include "hephaestus/telemetry/metrics/metric_record.h"
#include "hephaestus/telemetry/metrics/metric_sink.h"

namespace heph::conduit::detail {

[[nodiscard]] auto NodeBase::nodeName() const -> std::string {
  if (engine_ == nullptr) {
    return nodeName("");
  }
  return nodeName(engine_->prefix());
}

auto NodeBase::operationStart(bool has_period) -> ClockT::time_point {
  auto start_at = nextStartTime(has_period);
  last_steady_ = std::chrono::steady_clock::now();
  last_system_ = ClockT::now();
  if (iteration_ == 0) {
    start_time_ = ClockT::now();
  }
  return start_at;
}

void NodeBase::operationEnd() {
  auto steady_now = std::chrono::steady_clock::now();
  auto system_now = std::chrono::system_clock::now();

  auto tick_duration = steady_now - last_steady_;
  auto clock_drift =
      std::chrono::duration_cast<std::chrono::microseconds>((system_now - last_system_) - tick_duration);

  std::chrono::microseconds period_drift{ 0 };
  auto period = nodePeriod();
  if (period != std::chrono::nanoseconds{ 0 }) {
    period_drift = std::chrono::duration_cast<std::chrono::microseconds>(
        ClockT::now() - (start_time_ + (iteration_ * period)));
  }

  if (iteration_ > 0) {
    heph::telemetry::record([name = nodeName(), timestamp = system_now,
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

  auto period = std::chrono::duration_cast<ClockT::duration>(nodePeriod());
  auto next_time_point = start_time_ + (iteration_ * period);
  if (next_time_point < now) {
    auto last_duration = now - last_system_;
    // reset start time to avoid errors propagating if the deadline miss was too large
    start_time_ = now;
    iteration_ = 0;

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

auto NodeBase::scheduler() const -> concurrency::Context::Scheduler {
  if (engine_ == nullptr) {
    return { nullptr };
  }
  return engine_->scheduler();
}

auto NodeBase::enginePrefix() const -> std::string {
  if (engine_ == nullptr) {
    return "";
  }
  return engine_->prefix();
}

auto NodeBase::runsOnEngine() const -> bool {
  if (engine_ == nullptr) {
    return true;
  }
  return engine_->isCurrent();
}
}  // namespace heph::conduit::detail
#endif
