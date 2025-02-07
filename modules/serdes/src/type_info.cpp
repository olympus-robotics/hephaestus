//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/serdes/type_info.h"

#include <string>

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/utils/exception.h"

namespace heph::serdes {
// TODO(@filippobrizzi): add tests.

auto TypeInfo::toJson() const -> std::string {
  nlohmann::json data;
  data["name"] = name;
  data["schema"] = schema;
  data["serialization"] = magic_enum::enum_name(serialization);
  data["original_type"] = original_type;

  return data.dump();
}

auto TypeInfo::fromJson(const std::string& info) -> TypeInfo {
  auto data = nlohmann::json::parse(info);
  auto serialization_str = data["serialization"].get<std::string>();
  auto serialization = magic_enum::enum_cast<TypeInfo::Serialization>(serialization_str);
  throwExceptionIf<InvalidDataException>(
      !serialization.has_value(),
      fmt::format("failed to convert {} to Serialization enum", serialization_str));
  return { .name = data["name"],
           .schema = data["schema"],
           .serialization = serialization.value(),  // NOLINT(bugprone-unchecked-optional-access)
           .original_type = data["original_type"] };
}

auto ServiceTypeInfo::toJson() const -> std::string {
  nlohmann::json data;
  data["request"] = nlohmann::json::parse(request.toJson());
  data["reply"] = nlohmann::json::parse(reply.toJson());

  return data.dump();
}

auto ServiceTypeInfo::fromJson(const std::string& info) -> ServiceTypeInfo {
  auto data = nlohmann::json::parse(info);
  return { .request = TypeInfo::fromJson(data["request"].dump()),
           .reply = TypeInfo::fromJson(data["reply"].dump()) };
}
}  // namespace heph::serdes
