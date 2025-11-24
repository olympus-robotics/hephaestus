//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#pragma once

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/serdes/protobuf/buffers.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/protobuf.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/serdes/protobuf/protobuf_internal.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/utils.h"

namespace heph::serdes::protobuf {

template <class T>
auto serialize(const T& data) -> std::vector<std::byte> {
  return internal::serialize<T, typename ProtoAssociation<T>::Type>(data);
}

template <class T>
[[nodiscard]] auto serializeToJSON(const T& data) -> std::string {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  toProto(proto, data);

  std::string json_string;
  google::protobuf::util::JsonPrintOptions options;
  options.unquote_int64_if_possible = true;
  options.preserve_proto_field_names = true;
  options.add_whitespace = true;
  options.always_print_fields_with_no_presence = true;
  auto status = google::protobuf::util::MessageToJsonString(proto, &json_string, options);
  panicIf(!status.ok(), "failed to convert proto message to JSON with error: {}", status.message());

  return json_string;
}

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  toProto(proto, data);

  std::string txt_string;
  const auto success = google::protobuf::TextFormat::PrintToString(proto, &txt_string);
  panicIf(!success, "failed to serialize message to text");

  return txt_string;
}

template <class T>
void deserialize(std::span<const std::byte> buffer, T& data) {
  DeserializerBuffer des_buffer{ buffer };
  internal::fromProtobuf(des_buffer, data);
}

template <class T>
void deserializeFromJSON(std::string_view buffer, T& data) {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;
  auto status = google::protobuf::util::JsonStringToMessage(buffer, &proto);
  panicIf(!status.ok(), "failed to load proto message from JSON with error: {}", status.message());

  fromProto(proto, data);
}

template <class T>
void deserializeFromText(std::string_view buffer, T& data) {
  using Proto = ProtoAssociation<T>::Type;
  Proto proto;

  const auto success = google::protobuf::TextFormat::ParseFromString(buffer, &proto);
  panicIf(!success, "failed to deserialize message from text");

  fromProto(proto, data);
}

template <class T>
auto getTypeInfo() -> TypeInfo {
  using ProtoT = ProtoAssociation<T>::Type;
  return internal::getProtoTypeInfo<ProtoT>(utils::getTypeName<T>());
}

}  // namespace heph::serdes::protobuf

#define HEPHAESTUS_INSTANTIATE_PROTO_SERIALIZERS(T)                                                          \
  template std::vector<std::byte> heph::serdes::protobuf::serialize<T>(T const&);                            \
  template std::string heph::serdes::protobuf::serializeToJSON<T>(T const&);                                 \
  template std::string heph::serdes::protobuf::serializeToText<T>(T const&);                                 \
  template void heph::serdes::protobuf::deserialize<T>(std::span<const std::byte>, T&);                      \
  template void heph::serdes::protobuf::deserializeFromJSON<T>(std::string_view, T&);                        \
  template void heph::serdes::protobuf::deserializeFromText<T>(std::string_view, T&);                        \
  template heph::serdes::TypeInfo heph::serdes::protobuf::getTypeInfo<T>()
