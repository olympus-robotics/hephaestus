//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/bounds.h"
#include "hephaestus/types/proto/bounds.pb.h"
#include "hephaestus/types_proto/numeric_value.h"

namespace heph::serdes::protobuf {
template <heph::types::IsNumeric T>
struct ProtoAssociation<types::Bounds<T>> {
  using Type = types::proto::Bounds;
};
}  // namespace heph::serdes::protobuf

namespace heph::types {
namespace internal {
[[nodiscard]] auto getAsProto(const BoundsType& bounds_type) -> proto::BoundsType;
[[nodiscard]] auto getFromProto(const proto::BoundsType& proto_bounds_type) -> BoundsType;
}  // namespace internal

template <IsNumeric T>
auto toProto(proto::Bounds& proto_bounds, const Bounds<T>& bounds) -> void;
template <IsNumeric T>
auto fromProto(const proto::Bounds& proto_bounds, Bounds<T>& bounds) -> void;

//=================================================================================================
// Implementation
//=================================================================================================

template <IsNumeric T>
auto toProto(proto::Bounds& proto_bounds, const Bounds<T>& bounds) -> void {
  toProto(*proto_bounds.mutable_lower_bound(), bounds.lower_bound);
  toProto(*proto_bounds.mutable_upper_bound(), bounds.upper_bound);
  proto_bounds.set_bounds_type(internal::getAsProto(bounds.bounds_type));
}

template <IsNumeric T>
auto fromProto(const proto::Bounds& proto_bounds, Bounds<T>& bounds) -> void {
  fromProto(proto_bounds.lower_bound(), bounds.lower_bound);
  fromProto(proto_bounds.upper_bound(), bounds.upper_bound);
  bounds.bounds_type = internal::getFromProto(proto_bounds.bounds_type());
}

}  // namespace heph::types
