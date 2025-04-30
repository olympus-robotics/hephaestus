//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/uuid_v4.h"

#include "hephaestus/types/proto/uuid_v4.pb.h"
#include "hephaestus/types/uuid_v4.h"

namespace heph::types {

void toProto(proto::UuidV4& proto_uuid, const UuidV4& uuid) {
  proto_uuid.set_high(uuid.high);
  proto_uuid.set_low(uuid.low);
}

void fromProto(const proto::UuidV4& proto_uuid, UuidV4& uuid) {
  uuid.high = proto_uuid.high();
  uuid.low = proto_uuid.low();
}

}  // namespace heph::types
