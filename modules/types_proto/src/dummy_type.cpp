//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types_proto/dummy_type.h"

#include <cstddef>

#include "hephaestus/serdes/protobuf/containers.h"
#include "hephaestus/serdes/protobuf/enums.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types/proto/dummy_type.pb.h"

namespace heph::types {

//=================================================================================================
// DummyPrimitivesType
//=================================================================================================
void toProto(proto::DummyPrimitivesType& proto_dummy_primitives_type,
             const DummyPrimitivesType& dummy_primitives_type) {
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

void fromProto(const proto::DummyPrimitivesType& proto_dummy_primitives_type,
               DummyPrimitivesType& dummy_primitives_type) {
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

void toProto(proto::DummyType& proto_dummy_type, const DummyType& dummy_type) {
  toProto(*proto_dummy_type.mutable_dummy_primitives_type(), dummy_type.dummy_primitives_type);

  proto_dummy_type.set_internal_dummy_enum(
      serdes::protobuf::toProtoEnum<proto::DummyType::InternalDummyEnum>(dummy_type.internal_dummy_enum));
  proto_dummy_type.set_external_dummy_enum(
      serdes::protobuf::toProtoEnum<proto::DummyTypeExternalDummyEnum>(dummy_type.external_dummy_enum));

  proto_dummy_type.set_dummy_string(dummy_type.dummy_string);

  serdes::protobuf::toProto(*proto_dummy_type.mutable_dummy_vector(), dummy_type.dummy_vector);
  serdes::protobuf::toProto(*proto_dummy_type.mutable_dummy_vector_encapsulated(),
                            dummy_type.dummy_vector_encapsulated);

  serdes::protobuf::toProto(*proto_dummy_type.mutable_dummy_array(), dummy_type.dummy_array);
  serdes::protobuf::toProto(*proto_dummy_type.mutable_dummy_array_encapsulated(),
                            dummy_type.dummy_array_encapsulated);
}

void fromProto(const proto::DummyType& proto_dummy_type, DummyType& dummy_type) {
  fromProto(proto_dummy_type.dummy_primitives_type(), dummy_type.dummy_primitives_type);

  serdes::protobuf::fromProto(proto_dummy_type.internal_dummy_enum(), dummy_type.internal_dummy_enum);
  serdes::protobuf::fromProto(proto_dummy_type.external_dummy_enum(), dummy_type.external_dummy_enum);

  dummy_type.dummy_string = proto_dummy_type.dummy_string();

  serdes::protobuf::fromProto(proto_dummy_type.dummy_vector(), dummy_type.dummy_vector);
  serdes::protobuf::fromProto(proto_dummy_type.dummy_vector_encapsulated(),
                              dummy_type.dummy_vector_encapsulated);

  serdes::protobuf::fromProto(proto_dummy_type.dummy_array(), dummy_type.dummy_array);
  serdes::protobuf::fromProto(proto_dummy_type.dummy_array_encapsulated(),
                              dummy_type.dummy_array_encapsulated);
}

}  // namespace heph::types
