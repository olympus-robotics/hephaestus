//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/serdes/type_info.h"

#include <string>

#include <magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/error_handling/panic.h"

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
  panicIf(!serialization.has_value(), "failed to convert {} to Serialization enum", serialization_str);
  return { .name = data["name"],
           .schema = data["schema"],
           .serialization = serialization.value(),  // NOLINT(bugprone-unchecked-optional-access)
           .original_type = data["original_type"] };
}

auto TypeInfo::isValid() const -> bool {
  switch (serialization) {
    case Serialization::TEXT:
    case Serialization::JSON:
      // For TEXT and JSON, no additional information is technically required.
      return true;
    case Serialization::PROTOBUF:
      // For PROTOBUF, name and schema must not be empty.
      // The schema should also be a valid protobuf definition, but we will not check that here.
      return !schema.empty() && !name.empty();
  }
  return false;
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

auto ServiceTypeInfo::isValid() const -> bool {
  return request.isValid() && reply.isValid();
}

auto ActionServerTypeInfo::toJson() const -> std::string {
  nlohmann::json data;
  data["request"] = nlohmann::json::parse(request.toJson());
  data["reply"] = nlohmann::json::parse(reply.toJson());
  data["status"] = nlohmann::json::parse(status.toJson());

  return data.dump();
}

auto ActionServerTypeInfo::fromJson(const std::string& info) -> ActionServerTypeInfo {
  auto data = nlohmann::json::parse(info);
  return { .request = TypeInfo::fromJson(data["request"].dump()),
           .reply = TypeInfo::fromJson(data["reply"].dump()),
           .status = TypeInfo::fromJson(data["status"].dump()) };
}

}  // namespace heph::serdes
