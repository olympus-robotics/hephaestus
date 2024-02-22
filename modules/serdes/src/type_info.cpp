//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/serdes/type_info.h"

#include <fmt/core.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include "eolo/base/exception.h"

namespace eolo::serdes {
// TODO(filippo): add tests.

auto TypeInfo::toJson() const -> std::string {
  nlohmann::json data;
  data["name"] = name;
  data["schema"] = schema;
  data["serialization"] = magic_enum::enum_name(serialization);
  auto res = data.dump();
  return data.dump();
}

auto TypeInfo::fromJson(const std::string& info) -> TypeInfo {
  auto data = nlohmann::json::parse(info);
  auto serialization_str = data["serialization"].get<std::string>();
  auto serialization = magic_enum::enum_cast<TypeInfo::Serialization>(serialization_str);
  throwExceptionIf<InvalidDataException>(
      !serialization.has_value(),
      std::format("failed to convert {} to Serialization enum", serialization_str));
  return { .name = data["name"], .schema = data["schema"], .serialization = serialization.value() };
}
}  // namespace eolo::serdes
