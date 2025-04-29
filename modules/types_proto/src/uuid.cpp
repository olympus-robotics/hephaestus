//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/uuid.h"

#include "hephaestus/types/proto/uuid.pb.h"

namespace heph::types {

void toProto(proto::Uuid& proto_uuid, Uuid uuid) {
  proto_uuid.set_high(uuid.high);
  proto_uuid.set_low(uuid.low);
}

void fromProto(const proto::Uuid& proto_uuid, Uuid& uuid) {
  uuid.high = proto_uuid.high();
  uuid.low = proto_uuid.low();
}

}  // namespace heph::types
