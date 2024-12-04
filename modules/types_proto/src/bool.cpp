//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/bool.h"

#include "hephaestus/types/proto/bool.pb.h"

namespace heph::types::proto {

auto toProto(Bool& proto_value, bool value) -> void {
  proto_value.set_value(value);
}

auto fromProto(const Bool& proto_value, bool& value) -> void {
  value = proto_value.value();
}

}  // namespace heph::types::proto
