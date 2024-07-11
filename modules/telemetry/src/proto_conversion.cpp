//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "proto_conversion.h"

#include <chrono>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>

#include "hephaestus/telemetry/proto/log_entry.pb.h"
#include "hephaestus/telemetry/sink.h"

namespace heph::telemetry {
namespace {
// TODO: move this in a common place
void toProto(google::protobuf::Timestamp& proto_timestamp, const ClockT::time_point& timestamp) {
  const auto timestamp_proto = google::protobuf::util::TimeUtil::NanosecondsToTimestamp(
      std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count());
  proto_timestamp.set_nanos(timestamp_proto.nanos());
  proto_timestamp.set_seconds(timestamp_proto.seconds());
}

void fromProto(const google::protobuf::Timestamp& proto_timestamp, ClockT::time_point& timestamp) {
  const auto nanoseconds =
      std::chrono::nanoseconds{ google::protobuf::util::TimeUtil::TimestampToNanoseconds(proto_timestamp) };
  timestamp = ClockT::time_point{ std::chrono::duration_cast<ClockT::duration>(nanoseconds) };
}

}  // namespace

void toProto(proto::MetricEntry& proto_log_entry, const MetricEntry& log_entry) {
  proto_log_entry.set_component(log_entry.component);
  proto_log_entry.set_tag(log_entry.tag);
  toProto(*proto_log_entry.mutable_log_timestamp(), log_entry.log_timestamp);
  proto_log_entry.set_json_values(log_entry.json_values);
}

void fromProto(const proto::MetricEntry& proto_log_entry, MetricEntry& log_entry) {
  log_entry.component = proto_log_entry.component();
  log_entry.tag = proto_log_entry.tag();
  fromProto(proto_log_entry.log_timestamp(), log_entry.log_timestamp);
  log_entry.json_values = proto_log_entry.json_values();
}

}  // namespace heph::telemetry
