//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/serdes/dynamic_deserializer.h"

#include "hephaestus/serdes/type_info.h"

namespace heph::serdes {

void DynamicDeserializer::registerSchema(const TypeInfo& type_info) {
  switch (type_info.serialization) {
    case TypeInfo::Serialization::PROTOBUF:
      proto_deserializer_.registerSchema(type_info);
      [[fallthrough]];
    case TypeInfo::Serialization::JSON:
      [[fallthrough]];
    case TypeInfo::Serialization::TEXT:
      type_to_serialization_[type_info.name] = type_info.serialization;
  }
}

auto DynamicDeserializer::toJson(const std::string& type, std::span<const std::byte> data) -> std::string {
  switch (type_to_serialization_[type]) {
    case TypeInfo::Serialization::PROTOBUF:
      return proto_deserializer_.toJson(type, data);
    case TypeInfo::Serialization::JSON:
      [[fallthrough]];
    case TypeInfo::Serialization::TEXT:
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return std::string{ reinterpret_cast<const char*>(data.data()), data.size() };
  }
}
}  // namespace heph::serdes
