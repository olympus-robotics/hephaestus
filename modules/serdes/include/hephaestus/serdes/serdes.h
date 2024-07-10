//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <filesystem>
#include <type_traits>

#include "hephaestus/serdes/protobuf/protobuf.h"

namespace heph::serdes {

template <class T>
concept ProtobufSerializable = protobuf::ProtobufMessage<typename protobuf::ProtoAssociation<T>::Type>;

template <class T>
concept JSONSerializable = ProtobufSerializable<T>;  // Add new options when we extend support.

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

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

template <class T>
auto serialize(const T& data) -> std::vector<std::byte> {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::serialize(data);
  } else {
    static_assert(ProtobufSerializable<T>, "No serialization supported");
  }
}

template <class T>
auto serializeToJSON(const T& data) -> std::string {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::serializeToJSON(data);
  } else {
    static_assert(ProtobufSerializable<T>, "No serialization to JSON supported");
  }
}

template <class T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::serializeToText(data);
  } else {
    static_assert(ProtobufSerializable<T>, "No serialization to text supported");
  }
}

template <class T>
auto deserialize(std::span<const std::byte> buffer, T& data) -> void {
  if constexpr (ProtobufSerializable<T>) {
    protobuf::deserialize(buffer, data);
  } else {
    static_assert(ProtobufSerializable<T>, "No deserialization supported");
  }
}

template <class T>
auto deserializeFromJSON(std::string_view buffer, T& data) -> void {
  if constexpr (ProtobufSerializable<T>) {
    protobuf::deserializeFromJSON(buffer, data);
  } else {
    static_assert(ProtobufSerializable<T>, "No deserialization from JSON supported");
  }
}

template <class T>
auto deserializeFromText(std::string_view buffer, T& data) -> void {
  if constexpr (ProtobufSerializable<T>) {
    protobuf::deserializeFromText(buffer, data);
  } else {
    static_assert(ProtobufSerializable<T>, "No deserialization from text supported");
  }
}

template <class T>
auto getSerializedTypeInfo() -> TypeInfo {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::getTypeInfo<T>();
  } else {
    static_assert(ProtobufSerializable<T>, "No serialization supported");
  }
}

}  // namespace heph::serdes
