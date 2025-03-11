//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

#include "hephaestus/serdes/protobuf/buffers.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/utils.h"

namespace heph::serdes::protobuf::internal {

template <class T, class ProtoT>
void toProtobuf(SerializerBuffer& buffer, const T& data) {
  ProtoT proto;
  toProto(proto, data);
  buffer.serialize(proto);
}

template <class T, class ProtoT>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte> {
  SerializerBuffer buffer{};
  internal::toProtobuf<T, ProtoT>(buffer, data);
  return std::move(buffer).extractSerializedData();
}

template <class T>
void fromProtobuf(DeserializerBuffer& buffer, T& data) {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  auto res = buffer.deserialize(proto);
  panicIf(!res, fmt::format("Failed to parse {} from incoming buffer", utils::getTypeName<T>()));

  fromProto(proto, data);
}

/// Builds a FileDescriptorSet of this descriptor and all transitive dependencies, for use as a
/// channel schema. The descriptor can be obtained via `ProtoType::descriptor()`.
auto buildFileDescriptorSet(const google::protobuf::Descriptor* toplevel_descriptor)
    -> google::protobuf::FileDescriptorSet;

template <class ProtoT>
[[nodiscard]] auto getProtoTypeInfo(std::string original_type) -> TypeInfo {
  auto proto_descriptor = ProtoT::descriptor();
  auto file_descriptor = buildFileDescriptorSet(proto_descriptor).SerializeAsString();

  std::vector<std::byte> schema(file_descriptor.size());
  std::transform(file_descriptor.begin(), file_descriptor.end(), schema.begin(),
                 [](char c) { return static_cast<std::byte>(c); });

  auto original_type_tmp = std::move(original_type);  // NOTE: this is need otherwise clang-tidy will complain
  return { .name = proto_descriptor->full_name(),
           .schema = schema,
           .serialization = TypeInfo::Serialization::PROTOBUF,
           .original_type = std::move(original_type_tmp) };
}

}  // namespace heph::serdes::protobuf::internal
