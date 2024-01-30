//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <eolo/base/exception.h>

#include "eolo/serdes/protobuf/buffers.h"

namespace eolo::serdes::protobuf {
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

template <class Proto, class T>
void toProtobuf(serdes::protobuf::SerializerBuffer& buffer, const T& data)
  requires ProtobufConvertible<Proto, T>
{
  Proto proto;
  toProto(proto, data);
  buffer.serialize(proto);
}

template <class Proto, class T>
void fromProtobuf(serdes::protobuf::DeserializerBuffer& buffer, T& data)
  requires ProtobufConvertible<Proto, T>
{
  Proto proto;
  auto res = buffer.deserialize(proto);
  throwExceptionIf<InvalidDataException>(!res, "Failed to parse proto::pose from incoming buffer");

  fromProto(proto, data);
}

}  // namespace eolo::serdes::protobuf
