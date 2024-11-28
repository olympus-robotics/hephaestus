//=================================================================================================
// PIPPO © Copyright, 2015-2023. All Rights Reserved.
//
// This file is part of the RoboCool project
//=================================================================================================

#include "hephaestus/utils/timing/stop_watch.h"

#include <chrono>
#include <cstddef>
#include <optional>

namespace heph::utils::timing {

StopWatch::StopWatch() {
  reset();
}

void StopWatch::start() {
  if (lap_start_time_.has_value() || !initial_start_time_.has_value()) {
    return;
  }

  lap_start_time_ = ClockT::now();
  if (initial_start_time_->time_since_epoch().count() == 0) {
    initial_start_time_ = lap_start_time_.value();
  }
}

void StopWatch::reset() {
  lap_start_time_ = std::nullopt;
  initial_start_time_ = std::nullopt;
  lap_stop_time_ = {};
  accumulated_time_ = {};
  lap_counter_ = 0;
}

auto StopWatch::getAccumulatedLapTimes() const -> ClockT::duration {
  return (lap_start_time_.has_value() ? (accumulated_time_ + (ClockT::now() - lap_start_time_.value())) :
                                        accumulated_time_);
}

auto StopWatch::getTotalElapsedTime() const -> ClockT::duration {
  return initial_start_time_.has_value() ? (ClockT::now() - initial_start_time_.value()) : ClockT::duration{};
}

auto StopWatch::getLastStopTimestamp() const -> ClockT::time_point {
  return lap_stop_time_;
}

auto StopWatch::getLapsCount() const -> std::size_t {
  return lap_counter_;
}

auto StopWatch::lapseImpl() const -> ClockT::duration {
  if (!lap_start_time_.has_value()) {
    return {};
  }

  return ClockT::now() - lap_start_time_.value();
}

auto StopWatch::stopImpl() -> ClockT::duration {
  if (!lap_start_time_.has_value()) {
    return {};
  }

  lap_stop_time_ = ClockT::now();
  const auto lap_time = lap_stop_time_ - lap_start_time_.value();
  lap_start_time_ = std::nullopt;
  accumulated_time_ += lap_time;
  ++lap_counter_;
  return lap_time;
}

}  // namespace heph::utils::timing
