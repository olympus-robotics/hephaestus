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
#include "hephaestus/telemetry_influxdb_sink/influxdb_metric_sink.h"
namespace {

struct ClockDrift {
  std::chrono::microseconds::rep scheduler_us;
  std::chrono::microseconds::rep system_clock_us;
};
// NOLINTNEXTLINE(misc-include-cleaner)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(ClockDrift, scheduler_us, system_clock_us);

}  // namespace

namespace heph::conduit::detail {

NodeBase::NodeBase() noexcept
  : last_steady_{ std::chrono::steady_clock::now() }, last_system_{ std::chrono::system_clock::now() } {
}

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

ExecutionStopWatch::ExecutionStopWatch(NodeBase* self)
  : self_(self), start_(std::chrono::high_resolution_clock::now()) {
  self->calculateClockDrift();
}

ExecutionStopWatch::~ExecutionStopWatch() noexcept {
  auto end = std::chrono::high_resolution_clock::now();
  self_->updateExecutionTime(end - start_);
}

void NodeBase::calculateClockDrift() {
  auto period = std::chrono::duration_cast<std::chrono::milliseconds>(nodePeriod());

  // If period is zero, don't do the detection...
  if (period == std::chrono::milliseconds{ 0 }) {
    return;
  }

  auto now_steady = std::chrono::steady_clock::now();
  auto now_system = std::chrono::system_clock::now();

  auto duration_steady = now_steady - last_steady_;
  auto duration_system = now_system - last_system_;

  // a positive duration drift indicates that the clock under consideration took longer than
  // expected, and vice versa
  auto drift_scheduling = std::chrono::duration_cast<std::chrono::microseconds>(duration_steady - period);
  auto drift_system_clock =
      std::chrono::duration_cast<std::chrono::microseconds>(duration_system - duration_steady);

  static auto component = fmt::format("conduit/{}/clock_drift", nodeName());

  heph::telemetry::record(component, fmt::format("period={}", period),
                          ClockDrift{
                              .scheduler_us = drift_scheduling.count(),
                              .system_clock_us = drift_system_clock.count(),
                          });
  last_steady_ = now_steady;
  last_system_ = now_system;
}
auto NodeBase::getStopToken() -> stdexec::inplace_stop_token {
  if (engine_ == nullptr) {
    return stdexec::inplace_stop_token{};
  }
  return engine_->getStopToken();
}
}  // namespace heph::conduit::detail
