//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace heph::serdes {

// TODO(filippo): I get the following compiler error, but it seems to me redundant to declare the constructor
// for empty init. Remove lint warning?
// NOLINTBEGIN(cppcoreguidelines-pro-type-member-init)
struct TypeInfo {
  enum class Serialization : uint8_t { TEXT = 0, JSON, PROTOBUF };

  std::string name;
  std::vector<std::byte> schema;
  Serialization serialization = Serialization::TEXT;

  std::string original_type;  /// The type that is serialized by this.

  [[nodiscard]] auto toJson() const -> std::string;
  [[nodiscard]] static auto fromJson(const std::string& info) -> TypeInfo;
};
// NOLINTEND(cppcoreguidelines-pro-type-member-init)

}  // namespace heph::serdes
