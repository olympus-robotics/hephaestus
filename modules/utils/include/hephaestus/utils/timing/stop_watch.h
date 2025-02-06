//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <optional>

namespace heph::utils::timing {

/// StopWatch provides functionalities to measure elapsed time in different intervals.
///
/// start                stop   start        stop
///   |  lapse  |  lapse  |       |   lapse   |
///   |   elapsed   |
///   |___________________|       |___________|
///             accumulatedLapsDuration
///
class StopWatch {
public:
  using ClockT = std::chrono::steady_clock;
  using DurationT = ClockT::duration;
  using NowFunctionPtr = std::chrono::steady_clock::time_point (*)();

  explicit StopWatch(NowFunctionPtr now_fn = &ClockT::now);

  /// Start new lap. Does nothing if already ticking.
  void start();

  /// Stop current lap and pause accumulating time.
  /// \return Lap time, which is time elapsed between most recent start() and this stop() cast to the
  /// desired duration.
  template <typename TargetDurationT = StopWatch::DurationT>
  [[nodiscard]] auto stop() -> TargetDurationT;

  /// \return Currently running lap time, measured from the last call to lapse.
  /// The first lap is measured from the last start timestamp:
  /// - time elapsed from max[most recent start(), most recent lapse()] to now.
  /// - Cast to desired duration.
  /// - Doesn't stop the watch.
  template <typename TargetDurationT = StopWatch::DurationT>
  [[nodiscard]] auto lapse() -> TargetDurationT;

  /// \return Elapsed time since the last start() cast to the desired duration.
  template <typename TargetDurationT = StopWatch::DurationT>
  [[nodiscard]] auto elapsed() -> TargetDurationT;

  /// Stop and reset accumulated information.
  void reset();

  /// \return Time accumulated across all laps since last reset().
  [[nodiscard]] auto accumulatedLapsDuration() const -> DurationT;

  /// \return Timestamp of the first `start()` call after the last `reset()`.
  [[nodiscard]] auto initialStartTimestamp() const -> std::optional<typename ClockT::time_point>;

  /// \return the number of times the timer has been stopped and re-started.
  [[nodiscard]] auto lapsCount() const -> std::size_t;

private:
  [[nodiscard]] auto lapseImpl() -> DurationT;
  [[nodiscard]] auto stopImpl() -> DurationT;
  [[nodiscard]] auto elapsedImpl() -> DurationT;

private:
  NowFunctionPtr now_fn_;

  std::optional<typename ClockT::time_point> lap_start_timestamp_;      //!< Timestamp at start().
  std::optional<typename ClockT::time_point> initial_start_timestamp_;  //!< Timestamp at first start() after
                                                                        //!< reset().
  std::optional<typename ClockT::time_point> lapse_timestamp_;          //!< Timestamp at lapse().
  std::chrono::nanoseconds accumulated_time_{ 0 };  //!< The time accumulated between start() and
                                                    //!< stop() calls, after the last reset() call.
  std::size_t lap_counter_{ 0 };  //!< Counts how many time stop() have been called after the last
                                  //!< reset().
};

template <typename TargetDurationT>
auto StopWatch::stop() -> TargetDurationT {
  return std::chrono::duration_cast<TargetDurationT>(stopImpl());
}

template <typename TargetDurationT>
auto StopWatch::lapse() -> TargetDurationT {
  return std::chrono::duration_cast<TargetDurationT>(lapseImpl());
}

template <typename TargetDurationT>
auto StopWatch::elapsed() -> TargetDurationT {
  return std::chrono::duration_cast<TargetDurationT>(elapsedImpl());
}

}  // namespace heph::utils::timing
