//=================================================================================================
// PIPPO © Copyright, 2015-2023. All Rights Reserved.
//
// This file is part of the RoboCool project
//=================================================================================================

#include "hephaestus/utils/timing/stop_watch.h"

#include <chrono>
#include <cstddef>
#include <tuple>

namespace heph::utils::timing {

StopWatch::StopWatch() {
  reset();
}

void StopWatch::start() {
  if (is_running_) {
    return;
  }
  lap_start_time_ = ClockT::now();
  if (initial_start_time_.time_since_epoch().count() == 0) {
    initial_start_time_ = lap_start_time_;
  }
  is_running_ = true;
}

void StopWatch::reset() {
  using namespace std::chrono_literals;
  std::ignore = stop();
  accumulated_time_ = {};
  lap_counter_ = 0;
  initial_start_time_ = {};
  lap_stop_time_ = {};
}

auto StopWatch::getAccumulatedLapTimes() const -> ClockT::duration {
  return (is_running_ ? (accumulated_time_ + (ClockT::now() - lap_start_time_)) : accumulated_time_);
}

auto StopWatch::getTotalElapsedTime() const -> ClockT::duration {
  return (is_running_ ? (ClockT::now() - initial_start_time_) : (lap_stop_time_ - initial_start_time_));
}

auto StopWatch::getLastStopTimestamp() const -> ClockT::time_point {
  return lap_stop_time_;
}

auto StopWatch::getNumLaps() const -> std::size_t {
  return lap_counter_;
}

auto StopWatch::lapseImpl() const -> ClockT::duration {
  if (!is_running_) {
    return {};
  }

  return std::chrono::duration_cast<DurationT>(ClockT::now() - lap_start_time_);
}

auto StopWatch::stopImpl() -> ClockT::duration {
  if (!is_running_) {
    return {};
  }

  lap_stop_time_ = ClockT::now();
  is_running_ = false;
  const auto lap_time = lap_stop_time_ - lap_start_time_;
  accumulated_time_ += lap_time;
  ++lap_counter_;
  return lap_time;
}

}  // namespace heph::utils::timing
