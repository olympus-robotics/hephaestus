//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/bool.pb.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<bool> {
  using Type = types::proto::Bool;
};
}  // namespace heph::serdes::protobuf

namespace heph::types::proto {

void toProto(Bool& proto_value, bool value);
void fromProto(const Bool& proto_value, bool& value);

}  // namespace heph::types::proto
