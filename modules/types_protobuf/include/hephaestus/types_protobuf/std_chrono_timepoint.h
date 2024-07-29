//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include <google/protobuf/timestamp.pb.h>

#include "hephaestus/serdes/protobuf/concepts.h"

namespace heph::types {
template <typename T>
concept IsStdClock =
    std::is_same_v<T, std::chrono::system_clock> || std::is_same_v<T, std::chrono::steady_clock> ||
    std::is_same_v<T, std::chrono::high_resolution_clock>;
}  // namespace heph::types

namespace heph::serdes::protobuf {
template <::heph::types::IsStdClock T>
struct ProtoAssociation<std::chrono::time_point<T>> {
  using Type = google::protobuf::Timestamp;
};
}  // namespace heph::serdes::protobuf

namespace heph::types {
template <IsStdClock T>
auto toProto(google::protobuf::Timestamp& proto_timestamp, const std::chrono::time_point<T>& timestamp)
    -> void;

template <IsStdClock T>
auto fromProto(const google::protobuf::Timestamp& proto_timestamp, std::chrono::time_point<T>& timestamp)
    -> void;
}  // namespace heph::types

//=================================================================================================
// Implementation
//=================================================================================================

namespace heph::types {

template <IsStdClock T>
auto toProto(google::protobuf::Timestamp& proto_timestamp, const std::chrono::time_point<T>& timestamp)
    -> void {
  using DurationT = typename T::duration;
  static constexpr auto DIGITS_AFTER_COMMA =
      std::chrono::system_clock::duration::period::den;  // sub-second precision
  static constexpr int64_t GIGA = 1'000'000'000;
  static constexpr int64_t SCALER = GIGA / DIGITS_AFTER_COMMA;

  auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(timestamp).time_since_epoch().count();
  auto subseconds_total = std::chrono::time_point_cast<DurationT>(timestamp).time_since_epoch().count();

  auto nanoseconds_after_comma = (subseconds_total % DIGITS_AFTER_COMMA) * SCALER;

  // Protobuf Timestamps are represented as seconds and nanoseconds.
  proto_timestamp.set_seconds(seconds);
  proto_timestamp.set_nanos(static_cast<int32_t>(nanoseconds_after_comma));
}

template <IsStdClock T>
auto fromProto(const google::protobuf::Timestamp& proto_timestamp, std::chrono::time_point<T>& timestamp)
    -> void {
  using DurationT = typename T::duration;
  static constexpr auto DIGITS_AFTER_COMMA =
      std::chrono::system_clock::duration::period::den;  // sub-second precision
  static constexpr int64_t GIGA = 1'000'000'000;
  static constexpr int64_t SCALER = DIGITS_AFTER_COMMA / GIGA;

  // Protobuf Timestamps are represented as seconds and nanoseconds.
  auto seconds = std::chrono::seconds(proto_timestamp.seconds());
  auto nanoseconds = std::chrono::nanoseconds(proto_timestamp.nanos());

  timestamp = T(seconds + nanoseconds);
}

}  // namespace heph::types
