//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/protobuf.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::serdes {

template <class T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte>;

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string;

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void;

template <class T>
auto deserializeFromText(std::string_view buffer, T& data) -> void;

template <class T>
auto getSerializedTypeInfo() -> TypeInfo;

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

template <class T>
auto serialize(const T& data) -> std::vector<std::byte> {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::serialize(data);
  } else {
    static_assert(protobuf::ProtobufSerializable<T>, "No serialization supported");
  }
}

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::serializeToText(data);
  } else {
    static_assert(protobuf::ProtobufSerializable<T>, "No serialization to text supported");
  }
}

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    protobuf::deserialize(buffer, data);
  } else {
    static_assert(protobuf::ProtobufSerializable<T>, "No deserialization supported");
  }
}

template <class T>
auto deserializeFromText(std::string_view buffer, T& data) -> void {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    protobuf::deserializeFromText(buffer, data);
  } else {
    static_assert(protobuf::ProtobufSerializable<T>, "No deserialization from text supported");
  }
}

template <class T>
auto getSerializedTypeInfo() -> TypeInfo {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::getTypeInfo<T>();
  } else {
    static_assert(protobuf::ProtobufSerializable<T>, "No serialization supported");
  }
}

}  // namespace heph::serdes
