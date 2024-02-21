//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/common.h"

#include <nlohmann/json.hpp>

namespace eolo::ipc {
[[nodiscard]] auto toJson(const TypeInfo& info) -> std::string {
  nlohmann::json data;
  data["name"] = info.name;
  data["schema"] = info.schema;

  return data.dump();
}

[[nodiscard]] auto fromJson(const std::string& info) -> TypeInfo {
  auto data = nlohmann::json::parse(info);
  return { .name = data["name"], .schema = data["schema"] };
}
}  // namespace eolo::ipc
