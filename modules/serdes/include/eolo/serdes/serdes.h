//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <type_traits>

#include "eolo/serdes/protobuf/protobuf.h"

namespace eolo::serdes {

template <class T>
concept ProtobufSerializable = requires(T data) {
  { !std::is_same_v<typename protobuf::ProtoAssociation<T>::Type, void> };
};

// ProtoAssociation<T>::Type

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

}  // namespace eolo::serdes
