//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/primitive_types.h"

#include "hephaestus/types/proto/primitive_types.pb.h"

namespace heph::types::proto {

void toProto(Bool& proto_value, bool value) {
  proto_value.set_value(value);
}

void fromProto(const Bool& proto_value, bool& value) {
  value = proto_value.value();
}

void toProto(Int& proto_value, int value) {
  proto_value.set_value(value);
}
void fromProto(const Int& proto_value, int& value) {
  value = proto_value.value();
}

void toProto(Float& proto_value, float value) {
  proto_value.set_value(value);
}
void fromProto(const Float& proto_value, float& value) {
  value = proto_value.value();
}

void toProto(Double& proto_value, double value) {
  proto_value.set_value(value);
}
void fromProto(const Double& proto_value, double& value) {
  value = proto_value.value();
}

}  // namespace heph::types::proto
