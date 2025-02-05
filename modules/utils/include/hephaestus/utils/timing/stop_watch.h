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
template <typename ClockT = std::chrono::steady_clock>
class StopWatch {
public:
  using DurationT = ClockT::duration;

  StopWatch();

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
  std::optional<typename ClockT::time_point> lap_start_timestamp_;      //!< Timestamp at start().
  std::optional<typename ClockT::time_point> initial_start_timestamp_;  //!< Timestamp at first start() after
                                                                        //!< reset().
  std::optional<typename ClockT::time_point> lapse_timestamp_;          //!< Timestamp at lapse().
  std::chrono::nanoseconds accumulated_time_{ 0 };  //!< The time accumulated between start() and
                                                    //!< stop() calls, after the last reset() call.
  std::size_t lap_counter_{ 0 };  //!< Counts how many time stop() have been called after the last
                                  //!< reset().
};

template <typename ClockT>
StopWatch<ClockT>::StopWatch() {
  reset();
}

template <typename ClockT>
void StopWatch<ClockT>::start() {
  if (lap_start_timestamp_.has_value()) {
    return;
  }

  lap_start_timestamp_ = ClockT::now();
  if (!initial_start_timestamp_.has_value()) {
    initial_start_timestamp_ = lap_start_timestamp_.value();
  }
}

template <typename ClockT>
void StopWatch<ClockT>::reset() {
  lap_start_timestamp_ = std::nullopt;
  initial_start_timestamp_ = std::nullopt;
  lapse_timestamp_ = std::nullopt;
  accumulated_time_ = {};
  lap_counter_ = 0;
}

template <typename ClockT>
template <typename TargetDurationT>
auto StopWatch<ClockT>::lapse() -> TargetDurationT {
  return std::chrono::duration_cast<TargetDurationT>(lapseImpl());
}

template <typename ClockT>
template <typename TargetDurationT>
auto StopWatch<ClockT>::stop() -> TargetDurationT {
  return std::chrono::duration_cast<TargetDurationT>(stopImpl());
}

template <typename ClockT>
template <typename TargetDurationT>
auto StopWatch<ClockT>::elapsed() -> TargetDurationT {
  return std::chrono::duration_cast<TargetDurationT>(elapsedImpl());
}

template <typename ClockT>
auto StopWatch<ClockT>::accumulatedLapsDuration() const -> ClockT::duration {
  return (lap_start_timestamp_.has_value() ?
              (accumulated_time_ + (ClockT::now() - lap_start_timestamp_.value())) :
              accumulated_time_);
}

template <typename ClockT>
auto StopWatch<ClockT>::initialStartTimestamp() const -> std::optional<typename ClockT::time_point> {
  return initial_start_timestamp_;
}

template <typename ClockT>
auto StopWatch<ClockT>::lapsCount() const -> std::size_t {
  return lap_counter_;
}

template <typename ClockT>
auto StopWatch<ClockT>::lapseImpl() -> ClockT::duration {
  if (!lap_start_timestamp_.has_value()) {
    return {};
  }

  auto lapse_start_timestamp =
      lapse_timestamp_.has_value() ? lapse_timestamp_.value() : lap_start_timestamp_.value();

  lapse_timestamp_ = ClockT::now();

  return lapse_timestamp_.value() - lapse_start_timestamp;
}

template <typename ClockT>
auto StopWatch<ClockT>::stopImpl() -> ClockT::duration {
  if (!lap_start_timestamp_.has_value()) {
    return {};
  }

  auto stop_timestamp = ClockT::now();
  const auto lap_time = stop_timestamp - lap_start_timestamp_.value();

  lap_start_timestamp_ = std::nullopt;
  lapse_timestamp_ = std::nullopt;
  accumulated_time_ += lap_time;
  ++lap_counter_;

  return lap_time;
}

template <typename ClockT>
auto StopWatch<ClockT>::elapsedImpl() -> ClockT::duration {
  if (!lap_start_timestamp_.has_value()) {
    return {};
  }

  return ClockT::now() - lap_start_timestamp_.value();
}

}  // namespace heph::utils::timing
