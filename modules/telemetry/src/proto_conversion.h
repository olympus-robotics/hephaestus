//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/telemetry/proto/log_entry.pb.h"
#include "hephaestus/telemetry/sink.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::telemetry::MetricEntry> {
  using Type = heph::telemetry::proto::MetricEntry;
};
}  // namespace heph::serdes::protobuf

namespace heph::telemetry {
void toProto(proto::MetricEntry& proto_log_entry, const MetricEntry& log_entry);

void fromProto(const proto::MetricEntry& proto_log_entry, MetricEntry& log_entry);

}  // namespace heph::telemetry
