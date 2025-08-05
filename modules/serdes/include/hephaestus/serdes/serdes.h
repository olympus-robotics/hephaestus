//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/protobuf/protobuf.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::serdes {
/// Serdes module provides generic support for serialization and deserialization of data types.
/// It is built to support any serialization library via template specialization.
/// Right now it only support Protobuf, but can be easily extended to support other libraries by simply
/// writing a new concept for your serialization library.

template <typename T>
static inline constexpr bool NOT_SERIALIZABLE = false;

template <typename T>
[[nodiscard]] auto serialize(const T& data) -> std::vector<std::byte> {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::serialize(data);
  } else {
    static_assert(NOT_SERIALIZABLE<T>,
                  "serialize is not implemented for this type, did you forget to include the header "
                  "with the serialization implementation?");
  }

  __builtin_unreachable();
}

template <typename T>
[[nodiscard]] auto serializeToText(const T& data) -> std::string {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::serializeToText(data);
  } else {
    static_assert(NOT_SERIALIZABLE<T>,
                  "serialize is not implemented for this type, did you forget to include the header "
                  "with the serialization implementation?");
  }

  __builtin_unreachable();
}

template <typename T>
void deserialize(std::span<const std::byte> buffer, T& data) {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    protobuf::deserialize(buffer, data);
    return;
  } else {
    static_assert(NOT_SERIALIZABLE<T>,
                  "serialize is not implemented for this type, did you forget to include the header "
                  "with the serialization implementation?");
  }
}

template <typename T>
void deserializeFromText(std::string_view buffer, T& data) {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    protobuf::deserializeFromText(buffer, data);
    return;
  } else {
    static_assert(NOT_SERIALIZABLE<T>,
                  "serialize is not implemented for this type, did you forget to include the header "
                  "with the serialization implementation?");
  }
}

template <typename T>
auto getSerializedTypeInfo() -> TypeInfo {
  if constexpr (protobuf::ProtobufSerializable<T>) {
    return protobuf::getTypeInfo<T>();
  } else {
    static_assert(NOT_SERIALIZABLE<T>,
                  "serialize is not implemented for this type, did you forget to include the header "
                  "with the serialization implementation?");
  }

  __builtin_unreachable();
}

}  // namespace heph::serdes
