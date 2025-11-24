//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types/proto/dummy_type.pb.h"

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<types::DummyPrimitivesType> {
  using Type = types::proto::DummyPrimitivesType;
};

template <>
struct ProtoAssociation<types::DummyType> {
  using Type = types::proto::DummyType;
};
}  // namespace heph::serdes::protobuf

namespace heph::types {
void toProto(proto::DummyPrimitivesType& proto_dummy_primitives_type,
             const DummyPrimitivesType& dummy_primitives_type);
void fromProto(const proto::DummyPrimitivesType& proto_dummy_primitives_type,
               DummyPrimitivesType& dummy_primitives_type);

void toProto(proto::DummyType& proto_dummy_type, const DummyType& dummy_type);
void fromProto(const proto::DummyType& proto_dummy_type, DummyType& dummy_type);
}  // namespace heph::types

#include "hephaestus/serdes/protobuf/protobuf.inl"

HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::DummyType);
HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(heph::types::DummyPrimitivesType);
