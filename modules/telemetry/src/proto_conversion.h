//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/telemetry/proto/log_entry.pb.h"
#include "hephaestus/telemetry/sink.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::telemetry::LogEntry> {
  using Type = heph::telemetry::proto::LogEntry;
};
}  // namespace heph::serdes::protobuf

namespace heph::telemetry {
void toProto(proto::LogEntry& proto_log_entry, const LogEntry& log_entry);

void fromProto(const proto::LogEntry& proto_log_entry, LogEntry& log_entry);

}  // namespace heph::telemetry
