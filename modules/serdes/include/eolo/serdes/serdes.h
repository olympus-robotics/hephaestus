//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <format>

#include "eolo/serdes/protobuf/buffers.h"
#include "eolo/serdes/protobuf/protobuf.h"

namespace eolo::serdes {

template <class T>
concept ProtobufSerializable = requires(T proto, protobuf::SerializerBuffer ser_buffer,
                                        protobuf::DeserializerBuffer des_buffer) {
  { toProtobuf(ser_buffer, proto) };
  { fromProtobuf(des_buffer, proto) };
};

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
auto deserialize(std::span<std::byte> buffer, T& data) -> void;

template <class T>
auto serialize(const T& data) -> std::vector<std::byte> {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::serialize(data);
  } else {
    static_assert(false, "a");
  }
}

template <class T>
auto deserialize(std::span<std::byte> buffer, T& data) -> void {
  if constexpr (ProtobufSerializable<T>) {
    protobuf::deserialize(buffer, data);
  }
}

}  // namespace eolo::serdes
