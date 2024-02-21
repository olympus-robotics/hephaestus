//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <eolo/base/exception.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <range/v3/range/conversion.hpp>

#include "eolo/serdes/protobuf/buffers.h"
#include "eolo/serdes/type_info.h"

namespace eolo::serdes::protobuf {

template <class T>
struct ProtoAssociation {
  using Type = void;
};

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void;

template <class T>
auto serialize(const T& data) -> std::vector<std::byte> {
  SerializerBuffer buffer{};
  toProtobuf(buffer, data);
  return std::move(buffer).exctractSerializedData();
}

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void {
  DeserializerBuffer des_buffer{ buffer };
  fromProtobuf(des_buffer, data);
}

// template <class Proto, class T>
// void toProtobuf(serdes::protobuf::SerializerBuffer& buffer, const T& data)
//   requires ProtobufConvertible<Proto, T>
// {
//   Proto proto;
//   toProto(proto, data);
//   buffer.serialize(proto);
// }

// template <class Proto, class T>
// void fromProtobuf(serdes::protobuf::DeserializerBuffer& buffer, T& data)
//   requires ProtobufConvertible<Proto, T>
// {
//   Proto proto;
//   auto res = buffer.deserialize(proto);
//   throwExceptionIf<InvalidDataException>(!res, "Failed to parse proto::pose from incoming buffer");

//   fromProto(proto, data);
// }
template <class T>
void toProtobuf(serdes::protobuf::SerializerBuffer& buffer, const T& data) {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  toProto(proto, data);
  buffer.serialize(proto);
}

template <class T>
void fromProtobuf(serdes::protobuf::DeserializerBuffer& buffer, T& data) {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  auto res = buffer.deserialize(proto);
  throwExceptionIf<InvalidDataException>(!res, "Failed to parse proto::pose from incoming buffer");

  fromProto(proto, data);
}

/// Builds a FileDescriptorSet of this descriptor and all transitive dependencies, for use as a
/// channel schema.
auto buildFileDescriptorSet(const google::protobuf::Descriptor* toplevel_descriptor)
    -> google::protobuf::FileDescriptorSet;

template <class T>
auto getTypeInfo() -> TypeInfo {
  using Proto = ProtoAssociation<T>::Type;
  auto proto_descriptor = Proto::descriptor();
  auto file_descriptor = buildFileDescriptorSet(proto_descriptor).SerializeAsString();

  std::vector<std::byte> schema(file_descriptor.size());
  std::transform(file_descriptor.begin(), file_descriptor.end(), schema.begin(),
                 [](char c) { return std::byte{ c }; });
  return { .name = proto_descriptor->full_name(),
           .schema = schema,
           .serialization = TypeInfo::Serialization::PROTOBUF };
}

}  // namespace eolo::serdes::protobuf
