//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/proto/primitive_types.pb.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<bool> {
  using Type = types::proto::Bool;
};

template <>
struct ProtoAssociation<int> {
  using Type = types::proto::Int;
};

template <>
struct ProtoAssociation<float> {
  using Type = types::proto::Float;
};

template <>
struct ProtoAssociation<double> {
  using Type = types::proto::Double;
};

}  // namespace heph::serdes::protobuf

namespace heph::types::proto {

/// \brief Convert a numeric value to a protobuf message. These functions are specialized for each numeric
/// type. The main usage is to allow for serialization of templated numeric types.
void toProto(Bool& proto_value, bool value);
void fromProto(const Bool& proto_value, bool& value);

void toProto(Int& proto_value, int value);
void fromProto(const Int& proto_value, int& value);

void toProto(Float& proto_value, float value);
void fromProto(const Float& proto_value, float& value);

void toProto(Double& proto_value, double value);
void fromProto(const Double& proto_value, double& value);

}  // namespace heph::types::proto
