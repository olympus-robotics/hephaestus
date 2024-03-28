//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <type_traits>

#include "hephaestus/serdes/protobuf/protobuf.h"

namespace heph::serdes {

template <class T>
concept ProtobufSerializable = protobuf::ProtobufMessage<typename protobuf::ProtoAssociation<T>::Type>;

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void;

template <class T>
auto serialize(const T& data) -> std::vector<std::byte> {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::serialize(data);
  } else {
    static_assert(false, "no serialization supported");
  }
}

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void {
  if constexpr (ProtobufSerializable<T>) {
    protobuf::deserialize(buffer, data);
  }
}

template <class T>
auto getSerializedTypeInfo() -> TypeInfo {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::getTypeInfo<T>();
  } else {
    static_assert(false, "no serialization supported");
  }
}

}  // namespace heph::serdes
