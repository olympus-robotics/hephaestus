//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/bool.h"

#include "hephaestus/types/proto/bool.pb.h"

namespace heph::types::proto {

void toProto(Bool& proto_value, bool value) {
  proto_value.set_value(value);
}

void fromProto(const Bool& proto_value, bool& value) {
  value = proto_value.value();
}

}  // namespace heph::types::proto
