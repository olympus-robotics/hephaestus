//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace heph::serdes {

struct TypeInfo {
  enum class Serialization : uint8_t { TEXT = 0, JSON, PROTOBUF };

  std::string name;
  std::vector<std::byte> schema;
  Serialization serialization = Serialization::TEXT;

  std::string original_type;  /// The type that is serialized by this.

  [[nodiscard]] auto toJson() const -> std::string;
  [[nodiscard]] static auto fromJson(const std::string& info) -> TypeInfo;
  [[nodiscard]] auto operator==(const TypeInfo&) const -> bool = default;
};

struct ServiceTypeInfo {
  TypeInfo request;
  TypeInfo reply;

  [[nodiscard]] auto toJson() const -> std::string;
  [[nodiscard]] static auto fromJson(const std::string& info) -> ServiceTypeInfo;
  [[nodiscard]] auto operator==(const ServiceTypeInfo&) const -> bool = default;
};

}  // namespace heph::serdes
