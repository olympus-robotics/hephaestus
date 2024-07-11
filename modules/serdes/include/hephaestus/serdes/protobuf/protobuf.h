//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <range/v3/range/conversion.hpp>

#include "hephaestus/serdes/protobuf/buffers.h"
#include "hephaestus/serdes/protobuf/protobuf_internal.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/utils.h"

namespace heph::serdes::protobuf {

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
[[nodiscard]] auto serializeToJSON(const T& data) -> std::string;

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string;

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void;

template <class T>
auto deserializeFromJSON(std::string_view buffer, T& data) -> void;

template <class T>
auto deserializeFromText(std::string_view buffer, T& data) -> void;

/// Create the type info for the serialized type associated with `T`.
template <class T>
[[nodiscard]] auto getTypeInfo() -> TypeInfo;

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

template <class T>
auto serialize(const T& data) -> std::vector<std::byte> {
  SerializerBuffer buffer{};
  internal::toProtobuf(buffer, data);
  return std::move(buffer).exctractSerializedData();
}

template <class T>
[[nodiscard]] auto serializeToJSON(const T& data) -> std::string {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  toProto(proto, data);

  std::string json_string;
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = true;
  options.always_print_fields_with_no_presence = true;
  auto status = google::protobuf::util::MessageToJsonString(proto, &json_string, options);
  throwExceptionIf<FailedSerdesOperation>(
      !status.ok(), fmt::format("failed to convert proto message to JSON with error: {}", status.message()));

  return json_string;
}

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  toProto(proto, data);

  std::string txt_string;
  const auto success = google::protobuf::TextFormat::PrintToString(proto, &txt_string);
  throwExceptionIf<FailedSerdesOperation>(!success, "failed to serialize message to text");

  return txt_string;
}

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void {
  DeserializerBuffer des_buffer{ buffer };
  internal::fromProtobuf(des_buffer, data);
}

template <class T>
auto deserializeFromJSON(std::string_view buffer, T& data) -> void {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  auto status = google::protobuf::util::JsonStringToMessage(buffer, &proto);
  throwExceptionIf<FailedSerdesOperation>(
      !status.ok(), fmt::format("failed to load proto message from JSON with error: {}", status.message()));

  fromProto(proto, data);
}

template <class T>
auto deserializeFromText(std::string_view buffer, T& data) -> void {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;

  const auto success = google::protobuf::TextFormat::ParseFromString(buffer, &proto);
  throwExceptionIf<FailedSerdesOperation>(!success, "failed to deserialize message from text");

  fromProto(proto, data);
}

template <class T>
auto getTypeInfo() -> TypeInfo {
  using Proto = ProtoAssociation<T>::Type;
  auto proto_descriptor = Proto::descriptor();
  auto file_descriptor = internal::buildFileDescriptorSet(proto_descriptor).SerializeAsString();

  std::vector<std::byte> schema(file_descriptor.size());
  std::transform(file_descriptor.begin(), file_descriptor.end(), schema.begin(),
                 [](char c) { return static_cast<std::byte>(c); });
  return { .name = proto_descriptor->full_name(),
           .schema = schema,
           .serialization = TypeInfo::Serialization::PROTOBUF,
           .original_type = utils::getTypeName<T>() };
}

}  // namespace heph::serdes::protobuf
