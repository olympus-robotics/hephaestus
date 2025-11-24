//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/uuid_v4.pb.h"
#include "hephaestus/types/uuid_v4.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<types::UuidV4> {
  using Type = types::proto::UuidV4;
};
}  // namespace heph::serdes::protobuf

namespace heph::types {

void toProto(proto::UuidV4& proto_uuid, const UuidV4& uuid);
void fromProto(const proto::UuidV4& proto_uuid, UuidV4& uuid);

}  // namespace heph::types

#include "hephaestus/serdes/protobuf/protobuf.inl"
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::UuidV4);
