//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <queue>

#include <fmt/format.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "hephaestus/base/exception.h"
#include "hephaestus/serdes/protobuf/buffers.h"
#include "hephaestus/utils/utils.h"

namespace heph::serdes::protobuf::internal {

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
  throwExceptionIf<InvalidDataException>(
      !res, fmt::format("Failed to parse {} from incoming buffer", utils::getTypeName<T>()));

  fromProto(proto, data);
}

/// Builds a FileDescriptorSet of this descriptor and all transitive dependencies, for use as a
/// channel schema. The descriptor can be obtained via `ProtoType::descriptor()`.
auto buildFileDescriptorSet(const google::protobuf::Descriptor* toplevel_descriptor)
    -> google::protobuf::FileDescriptorSet;

}  // namespace heph::serdes::protobuf::internal
