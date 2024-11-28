//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>

namespace heph::utils::timing {

class StopWatch {
public:
  using ClockT = std::chrono::steady_clock;
  using DurationT = ClockT::duration;

  StopWatch();

  /// Start new lap. Does nothing if already ticking.
  void start();

  /// Stop current lap and pause accumulating time.
  /// \return Lap time, which is time elapsed between most recent start() and this stop().
  [[nodiscard]] auto stop() -> DurationT;

  /// \return Currently running lap time (time elapsed from most recent start()).
  /// Doesn't stop the watch.
  [[nodiscard]] auto lapse() const -> DurationT;

  /// Stop and reset accumlated information.
  void reset();

  /// \return Time accumulated across all laps since last reset().
  [[nodiscard]] auto getAccumulatedLapTimes() const -> DurationT;

  /// \return Time elapsed since first start() after reset(). This includes time accumulated
  /// across laps and in between laps.
  [[nodiscard]] auto getTotalElapsedTime() const -> DurationT;

  /// \return Timestamp at most recent stop().
  /// \note (getLastStopTimestamp() - getTotalElapsedTime()) gives timestamp of the first call to
  /// start() after reset().
  [[nodiscard]] auto getLastStopTimestamp() const -> ClockT::time_point;

  /// \return the number of times the timer has been stopped and re-started.
  [[nodiscard]] auto getNumLaps() const -> std::size_t;

private:
  ClockT::time_point lap_start_time_;               //!< Timestamp at start()
  ClockT::time_point initial_start_time_;           //!< Timestamp at first start() after reset().
  ClockT::time_point lap_stop_time_;                //!< Timestamp at last stop()
  std::chrono::nanoseconds accumulated_time_{ 0 };  //!< The time accumulated between start() and
                                                    //!< stop() calls, after the last reset() call.
  bool is_running_{ false };                        //!< True if start() has been called, but not stop().
  std::size_t lap_counter_{ 0 };  //!< Counts how many time stop() have been called after the last
                                  //!< reset().
};
///@}
}  // namespace heph::utils::timing
