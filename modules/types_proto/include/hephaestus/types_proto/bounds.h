//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/enums.h"
#include "hephaestus/types/bounds.h"
#include "hephaestus/types/proto/bounds.pb.h"
#include "hephaestus/types_proto/numeric_value.h"  // used to serialize bounds
#include "hephaestus/utils/concepts.h"

namespace heph::serdes::protobuf {
template <NumericType T>
struct ProtoAssociation<types::Bounds<T>> {
  using Type = types::proto::Bounds;
};
}  // namespace heph::serdes::protobuf

namespace heph::types {

template <NumericType T>
void toProto(proto::Bounds& proto_bounds, const Bounds<T>& bounds);
template <NumericType T>
void fromProto(const proto::Bounds& proto_bounds, Bounds<T>& bounds);

//=================================================================================================
// Implementation
//=================================================================================================

template <NumericType T>
void toProto(proto::Bounds& proto_bounds, const Bounds<T>& bounds) {
  toProto(*proto_bounds.mutable_lower(), bounds.lower);
  toProto(*proto_bounds.mutable_upper(), bounds.upper);
  proto_bounds.set_type(serdes::protobuf::toProtoEnum<proto::BoundsType>(bounds.type));
}

template <NumericType T>
void fromProto(const proto::Bounds& proto_bounds, Bounds<T>& bounds) {
  fromProto(proto_bounds.lower(), bounds.lower);
  fromProto(proto_bounds.upper(), bounds.upper);
  serdes::protobuf::fromProto(proto_bounds.type(), bounds.type);
}

}  // namespace heph::types
