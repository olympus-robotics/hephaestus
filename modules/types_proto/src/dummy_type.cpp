//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/dummy_type.h"

#include <cstddef>
#include <vector>

#include "hephaestus/serdes/protobuf/enums.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types/proto/dummy_type.pb.h"

namespace heph::types {

//=================================================================================================
// Vector
//=================================================================================================
template <typename T, typename ProtoT>
auto toProto(google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, const std::vector<T>& vec)
    -> void {
  proto_repeated_field.Clear();  // Ensure that the repeated field is empty before adding elements.
  proto_repeated_field.Reserve(static_cast<int>(vec.size()));
  for (const auto& value : vec) {
    proto_repeated_field.Add(value);
  }
}

template <typename T, typename ProtoT>
auto fromProto(const google::protobuf::RepeatedField<ProtoT>& proto_repeated_field, std::vector<T>& vec)
    -> void {
  vec.clear();  // Ensure that the vector is empty before adding elements.
  vec.reserve(static_cast<size_t>(proto_repeated_field.size()));
  for (const auto& proto_value : proto_repeated_field) {
    vec.push_back(proto_value);
  }
}

//=================================================================================================
// DummyPrimitivesType
//=================================================================================================
auto toProto(proto::DummyPrimitivesType& proto_dummy_primitives_type,
             const DummyPrimitivesType& dummy_primitives_type) -> void {
  proto_dummy_primitives_type.set_dummy_bool(dummy_primitives_type.dummy_bool);

  proto_dummy_primitives_type.set_dummy_int8_t(dummy_primitives_type.dummy_int8_t);
  proto_dummy_primitives_type.set_dummy_int16_t(dummy_primitives_type.dummy_int16_t);
  proto_dummy_primitives_type.set_dummy_int32_t(dummy_primitives_type.dummy_int32_t);
  proto_dummy_primitives_type.set_dummy_int64_t(dummy_primitives_type.dummy_int64_t);

  proto_dummy_primitives_type.set_dummy_uint8_t(dummy_primitives_type.dummy_uint8_t);
  proto_dummy_primitives_type.set_dummy_uint16_t(dummy_primitives_type.dummy_uint16_t);
  proto_dummy_primitives_type.set_dummy_uint32_t(dummy_primitives_type.dummy_uint32_t);
  proto_dummy_primitives_type.set_dummy_uint64_t(dummy_primitives_type.dummy_uint64_t);

  proto_dummy_primitives_type.set_dummy_float(dummy_primitives_type.dummy_float);
  proto_dummy_primitives_type.set_dummy_double(dummy_primitives_type.dummy_double);
}

auto fromProto(const proto::DummyPrimitivesType& proto_dummy_primitives_type,
               DummyPrimitivesType& dummy_primitives_type) -> void {
  dummy_primitives_type.dummy_bool = proto_dummy_primitives_type.dummy_bool();

  dummy_primitives_type.dummy_int8_t =
      static_cast<decltype(dummy_primitives_type.dummy_int8_t)>(proto_dummy_primitives_type.dummy_int8_t());
  dummy_primitives_type.dummy_int16_t =
      static_cast<decltype(dummy_primitives_type.dummy_int16_t)>(proto_dummy_primitives_type.dummy_int16_t());
  dummy_primitives_type.dummy_int32_t = proto_dummy_primitives_type.dummy_int32_t();
  dummy_primitives_type.dummy_int64_t = proto_dummy_primitives_type.dummy_int64_t();

  dummy_primitives_type.dummy_uint8_t =
      static_cast<decltype(dummy_primitives_type.dummy_uint8_t)>(proto_dummy_primitives_type.dummy_uint8_t());
  dummy_primitives_type.dummy_uint16_t = static_cast<decltype(dummy_primitives_type.dummy_uint16_t)>(
      proto_dummy_primitives_type.dummy_uint16_t());
  dummy_primitives_type.dummy_uint32_t = proto_dummy_primitives_type.dummy_uint32_t();
  dummy_primitives_type.dummy_uint64_t = proto_dummy_primitives_type.dummy_uint64_t();

  dummy_primitives_type.dummy_float = proto_dummy_primitives_type.dummy_float();
  dummy_primitives_type.dummy_double = proto_dummy_primitives_type.dummy_double();
}

//=================================================================================================
// DummyType
//=================================================================================================

auto toProto(proto::DummyType& proto_dummy_type, const DummyType& dummy_type) -> void {
  toProto(*proto_dummy_type.mutable_dummy_primitives_type(), dummy_type.dummy_primitives_type);

  proto_dummy_type.set_internal_dummy_enum(
      serdes::protobuf::toProtoEnum<proto::DummyType::InternalDummyEnum>(dummy_type.internal_dummy_enum));
  proto_dummy_type.set_external_dummy_enum(
      serdes::protobuf::toProtoEnum<proto::DummyTypeExternalDummyEnum>(dummy_type.external_dummy_enum));

  proto_dummy_type.set_dummy_string(dummy_type.dummy_string);

  toProto(*proto_dummy_type.mutable_dummy_vector(), dummy_type.dummy_vector);
}

auto fromProto(const proto::DummyType& proto_dummy_type, DummyType& dummy_type) -> void {
  fromProto(proto_dummy_type.dummy_primitives_type(), dummy_type.dummy_primitives_type);

  serdes::protobuf::fromProto(proto_dummy_type.internal_dummy_enum(), dummy_type.internal_dummy_enum);
  serdes::protobuf::fromProto(proto_dummy_type.external_dummy_enum(), dummy_type.external_dummy_enum);

  dummy_type.dummy_string = proto_dummy_type.dummy_string();

  fromProto(proto_dummy_type.dummy_vector(), dummy_type.dummy_vector);
}

}  // namespace heph::types
