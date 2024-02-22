//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "eolo/base/exception.h"
#include "eolo/serdes/protobuf/buffers.h"

namespace eolo::serdes::protobuf::internal {

template <class T>
void toProtobuf(SerializerBuffer& buffer, const T& data) {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  toProto(proto, data);
  buffer.serialize(proto);
}

template <class T>
void fromProtobuf(DeserializerBuffer& buffer, T& data) {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  auto res = buffer.deserialize(proto);
  throwExceptionIf<InvalidDataException>(!res, "Failed to parse proto::pose from incoming buffer");

  fromProto(proto, data);
}

/// Builds a FileDescriptorSet of this descriptor and all transitive dependencies, for use as a
/// channel schema. The descriptor can be obtained via `ProtoType::descriptor()`.
auto buildFileDescriptorSet(const google::protobuf::Descriptor* toplevel_descriptor)
    -> google::protobuf::FileDescriptorSet;

}  // namespace eolo::serdes::protobuf::internal
