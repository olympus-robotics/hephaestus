//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

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

}  // namespace eolo::serdes::protobuf
