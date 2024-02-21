//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/serdes/type_info.h"

#include <fmt/core.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "eolo/base/exception.h"

namespace eolo::serdes {

[[nodiscard]] auto toJson(const TypeInfo& info) -> std::string {
  nlohmann::json data;
  data["name"] = info.name;
  data["schema"] = info.schema;
  data["serialization"] = magic_enum::enum_name(info.serialization);
  auto res = data.dump();
  return data.dump();
}

[[nodiscard]] auto fromJson(const std::string& info) -> TypeInfo {
  auto data = nlohmann::json::parse(info);
  auto serialization_str = data["serialization"].get<std::string>();
  auto serialization = magic_enum::enum_cast<TypeInfo::Serialization>(serialization_str);
  throwExceptionIf<InvalidDataException>(
      !serialization.has_value(),
      std::format("failed to convert {} to Serialization enum", serialization_str));
  return { .name = data["name"], .schema = data["schema"], .serialization = serialization.value() };
}
}  // namespace eolo::serdes
