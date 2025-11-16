//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <optional>

#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/scheduler.h"

namespace heph::conduit {
/// `Periodic` represents an input which is triggering at a fixed periodic duration.
/// Requires \ref setPeriodDuration to be called before usage.
class Periodic : public BasicInput {
public:
  Periodic() noexcept : BasicInput("periodic") {
  }

  /// needs to be called before `trigger` will be invoked.
  void setPeriodDuration(ClockT::duration period_duration) {
    period_duration_.emplace(period_duration);
  }

private:
  auto doTrigger(SchedulerT scheduler) -> SenderT final {
    if (!period_duration_.has_value()) {
      heph::panic("Period duration was not set");
    }
    auto now = ClockT::now();
    auto next_start_time = start_time_ + iteration_ * period_duration_.value();

    if (next_start_time < now) {
      start_time_ = now;
      iteration_ = 0;
      // TODO: missing deadline period warning...
      return scheduler.schedule() | stdexec::then([]() { return true; });
    }
    return scheduler.scheduleAt(next_start_time) | stdexec::then([]() { return true; });
  }

  void handleCompleted() final {
    ++iteration_;
  }

private:
  std::optional<ClockT::duration> period_duration_;
  ClockT::time_point start_time_{ ClockT::now() };
  ClockT::time_point last_trigger_time_{ ClockT::now() };
  std::size_t iteration_{ 0 };
};
}  // namespace heph::conduit
