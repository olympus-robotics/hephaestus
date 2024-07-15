//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <rfl.hpp>
#include <rfl/json.hpp>

#include "hephaestus/serdes/protobuf/concepts.h"
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

/// Serialize data to JSON.
/// This is achieved by checking at compile time what serialization capabilities are provided for the input
/// data. In order of priority, the following serialization methods are checked:
// - Protobuf
// - Custom `toJSON` / `fromJSON` functions
// - Nlohmann JSON serialization
// If none of the above are provided, we use reflection-based JSON serialization via `reflect-cpp` library.
template <class T>
[[nodiscard]] auto serializeToJSON(const T& data) -> std::string;

/// Deserialize data from JSON.
/// See `serializeToJSON` for more details.
template <class T>
auto deserializeFromJSON(std::string_view json, T& data) -> void;

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

template <typename T>
auto serializeToJSON(const T& data) -> std::string {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::serializeToJSON(data);
  } else if constexpr (HasJSONSerialization<T>) {
    return toJSON(data);
  } else if constexpr (HasNlohmannJSONSerialization<T>) {
    const nlohmann::json j = data;
    return j.dump();
  } else {
    return rfl::json::write(data);
  }
}

template <typename T>
auto deserializeFromJSON(std::string_view json, T& data) -> void {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    protobuf::deserializeFromJSON(json, data);
  } else if constexpr (HasJSONDeserialization<T>) {
    fromJSON(json, data);
  } else if constexpr (HasNlohmannJSONSerialization<T>) {
    data = nlohmann::json::parse(json).get<T>();
  } else {
    data = rfl::json::read<T>(json).value();
  }
}

}  // namespace heph::serdes
