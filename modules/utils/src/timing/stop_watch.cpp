//=================================================================================================
// PIPPO © Copyright, 2015-2023. All Rights Reserved.
//
// This file is part of the RoboCool project
//=================================================================================================

#include "hephaestus/utils/timing/stop_watch.h"

namespace heph::utils::timing {

StopWatch::StopWatch(NowFunctionPtr now_fn) : now_fn_(now_fn) {
  reset();
}

void StopWatch::start() {
  if (lap_start_timestamp_.has_value()) {
    return;
  }

  lap_start_timestamp_ = now_fn_();
  if (!initial_start_timestamp_.has_value()) {
    initial_start_timestamp_ = lap_start_timestamp_.value();
  }
}

void StopWatch::reset() {
  lap_start_timestamp_ = std::nullopt;
  initial_start_timestamp_ = std::nullopt;
  lapse_timestamp_ = std::nullopt;
  accumulated_time_ = {};
  lap_counter_ = 0;
}

auto StopWatch::accumulatedLapsDuration() const -> ClockT::duration {
  return (lap_start_timestamp_.has_value() ?
              (accumulated_time_ + (now_fn_() - lap_start_timestamp_.value())) :
              accumulated_time_);
}

auto StopWatch::initialStartTimestamp() const -> std::optional<typename ClockT::time_point> {
  return initial_start_timestamp_;
}

auto StopWatch::lapsCount() const -> std::size_t {
  return lap_counter_;
}

auto StopWatch::lapseImpl() -> ClockT::duration {
  if (!lap_start_timestamp_.has_value()) {
    return {};
  }

  auto lapse_start_timestamp =
      lapse_timestamp_.has_value() ? lapse_timestamp_.value() : lap_start_timestamp_.value();

  lapse_timestamp_ = now_fn_();

  return lapse_timestamp_.value() - lapse_start_timestamp;
}

auto StopWatch::stopImpl() -> ClockT::duration {
  if (!lap_start_timestamp_.has_value()) {
    return {};
  }

  auto stop_timestamp = now_fn_();
  const auto lap_time = stop_timestamp - lap_start_timestamp_.value();

  lap_start_timestamp_ = std::nullopt;
  lapse_timestamp_ = std::nullopt;
  accumulated_time_ += lap_time;
  ++lap_counter_;

  return lap_time;
}

auto StopWatch::elapsedImpl() -> ClockT::duration {
  if (!lap_start_timestamp_.has_value()) {
    return {};
  }

  return now_fn_() - lap_start_timestamp_.value();
}
}  // namespace heph::utils::timing
