//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <concepts>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/protobuf.h"

namespace heph::serdes {

template <typename T>
concept JSONSerializable = requires(const T& data) {
  { toJSON(data) } -> std::convertible_to<std::string>;
};

template <typename T>
concept JSONDeserializable = requires(T& mutable_data, std::string_view json) {
  { fromJSON(json, mutable_data) };
};

template <typename T>
concept NlohmannJSONSerializable = requires(const T& data) {
  { nlohmann::json(data) } -> std::convertible_to<nlohmann::json>;
};

/// Serialize data to JSON.
/// This is achieved by checking at compile time what serialization capabilities are provided for the input
/// data. In order of priority, the following serialization methods are checked:
// - Protobuf
// - Custom `toJSON` / `fromJSON` functions
// - Nlohmann JSON serialization
template <class T>
[[nodiscard]] auto serializeToJSON(const T& data) -> std::string;

/// Deserialize data from JSON.
/// See `serializeToJSON` for more details.
template <class T>
auto deserializeFromJSON(std::string_view json, T& data) -> void;

// -----------------------------------------------------------------------------------------------
// Specializations
// -----------------------------------------------------------------------------------------------

template <protobuf::ProtobufSerializable T>
auto serializeToJSON(const T& data) -> std::string {
  return protobuf::serializeToJSON(data);
}

template <JSONSerializable T>
auto serializeToJSON(const T& data) -> std::string {
  return toJSON(data);
}

template <NlohmannJSONSerializable T>
auto serializeToJSON(const T& data) -> std::string {
  const nlohmann::json j = data;
  return j.dump();
}

template <protobuf::ProtobufSerializable T>
auto deserializeFromJSON(std::string_view json, T& data) -> void {
  protobuf::deserializeFromJSON(json, data);
}

template <JSONDeserializable T>
auto deserializeFromJSON(std::string_view json, T& data) -> void {
  fromJSON(json, data);
}

template <NlohmannJSONSerializable T>
auto deserializeFromJSON(std::string_view json, T& data) -> void {
  data = nlohmann::json::parse(json).get<T>();
}

}  // namespace heph::serdes
