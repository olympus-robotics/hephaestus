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

template <typename T>
concept HasJSONSerialization = requires(const T& data) {
  { toJSON(data) } -> std::convertible_to<std::string>;
};

template <typename T>
concept HasJSONDeserialization = requires(T& mutable_data, std::string_view json) {
  { fromJSON(json, mutable_data) };
};

template <class T>
concept JSONSerializable = ProtobufSerializable<T> || HasJSONSerialization<T>;

template <class T>
concept JSONDeserializable = ProtobufSerializable<T> || HasJSONDeserialization<T>;

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

template <JSONSerializable T>
auto serializeToJSON(const T& data) -> std::string {
  if constexpr (ProtobufSerializable<T>) {
    return protobuf::serializeToJSON(data);
  } else if (HasJSONSerialization<T>) {
    return toJSON(data);
  } else {
    static_assert(JSONSerializable<T>, "No serialization to JSON supported");
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

template <JSONDeserializable T>
auto deserializeFromJSON(std::string_view buffer, T& data) -> void {
  if constexpr (ProtobufSerializable<T>) {
    protobuf::deserializeFromJSON(buffer, data);
  } else if (HasJSONDeserialization<T>) {
    fromJSON(buffer, data);
  } else {
    static_assert(JSONDeserializable<T>, "No deserialization from JSON supported");
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
