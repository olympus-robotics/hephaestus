//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/uuid.pb.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<Uuid> {
  using Type = types::proto::Uuid;
};
}  // namespace heph::serdes::protobuf

namespace heph::types {

void toProto(proto::Uuid& proto_uuid, Uuid uuid);
void fromProto(const proto::Uuid& proto_uuid, Uuid& uuid);

}  // namespace heph::types
