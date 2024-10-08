//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/bounds.h"
#include "hephaestus/types/proto/bounds.pb.h"
#include "hephaestus/types_proto/numeric_value.h"

namespace heph::serdes::protobuf {
template <NumericType T>
struct ProtoAssociation<types::Bounds<T>> {
  using Type = types::proto::Bounds;
};
}  // namespace heph::serdes::protobuf

namespace heph::types {

template <NumericType T>
auto toProto(proto::Bounds& proto_bounds, const Bounds<T>& bounds) -> void;
template <NumericType T>
auto fromProto(const proto::Bounds& proto_bounds, Bounds<T>& bounds) -> void;

//=================================================================================================
// Implementation
//=================================================================================================

namespace internal {
[[nodiscard]] auto getAsProto(const BoundsType& bounds_type) -> proto::BoundsType;
[[nodiscard]] auto getFromProto(const proto::BoundsType& proto_bounds_type) -> BoundsType;
}  // namespace internal

template <NumericType T>
auto toProto(proto::Bounds& proto_bounds, const Bounds<T>& bounds) -> void {
  toProto(*proto_bounds.mutable_lower(), bounds.lower);
  toProto(*proto_bounds.mutable_upper(), bounds.upper);
  proto_bounds.set_type(internal::getAsProto(bounds.type));
}

template <NumericType T>
auto fromProto(const proto::Bounds& proto_bounds, Bounds<T>& bounds) -> void {
  fromProto(proto_bounds.lower(), bounds.lower);
  fromProto(proto_bounds.upper(), bounds.upper);
  bounds.type = internal::getFromProto(proto_bounds.type());
}

}  // namespace heph::types
