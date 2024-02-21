//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <string>
#include <vector>

namespace eolo::serdes {

struct TypeInfo {
  enum class Serialization : uint8_t { TEXT = 0, JSON, PROTOBUF };

  std::string name;
  std::vector<std::byte> schema;
  Serialization serialization = Serialization::TEXT;
};

[[nodiscard]] auto toJson(const TypeInfo& info) -> std::string;
[[nodiscard]] auto fromJson(const std::string& info) -> TypeInfo;

}  // namespace eolo::serdes
