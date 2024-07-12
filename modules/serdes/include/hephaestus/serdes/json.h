//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <nlohmann/json.hpp>

#include "hephaestus/serdes/protobuf/protobuf.h"

namespace heph::serdes {

template <typename T>
concept HasJSONSerialization = requires(const T& data) {
  { toJSON(data) } -> std::convertible_to<std::string>;
};

template <typename T>
concept HasJSONDeserialization = requires(T& mutable_data, std::string_view json) {
  { fromJSON(json, mutable_data) };
};

template <typename T>
concept HasNlohmannJSONSerialization = requires(const T& data) {
  { nlohmann::json(data) } -> std::convertible_to<nlohmann::json>;
};

template <class T>
concept JSONSerializable =
    protobuf::ProtobufSerializable<T> || HasJSONSerialization<T> || HasNlohmannJSONSerialization<T>;

template <class T>
concept JSONDeserializable =
    protobuf::ProtobufSerializable<T> || HasJSONDeserialization<T> || HasNlohmannJSONSerialization<T>;

template <class T>
[[nodiscard]] auto serializeToJSON(const T& data) -> std::string;

template <class T>
auto deserializeFromJSON(std::string_view buffer, T& data) -> void;

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

template <JSONSerializable T>
auto serializeToJSON(const T& data) -> std::string {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::serializeToJSON(data);
  } else if constexpr (HasJSONSerialization<T>) {
    return toJSON(data);
  } else if constexpr (HasNlohmannJSONSerialization<T>) {
    const nlohmann::json j = data;
    return j.dump();
  } else {
    static_assert(JSONSerializable<T>, "No serialization to JSON supported");
  }
}

template <JSONDeserializable T>
auto deserializeFromJSON(std::string_view buffer, T& data) -> void {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    protobuf::deserializeFromJSON(buffer, data);
  } else if constexpr (HasJSONDeserialization<T>) {
    fromJSON(buffer, data);
  } else if constexpr (HasNlohmannJSONSerialization<T>) {
    data = nlohmann::json::parse(buffer).get<T>();
  } else {
    static_assert(JSONDeserializable<T>, "No deserialization from JSON supported");
  }
}

}  // namespace heph::serdes
