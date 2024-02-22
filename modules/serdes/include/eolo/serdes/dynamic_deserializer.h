//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>

#include "eolo/serdes/protobuf/dynamic_deserializer.h"
#include "eolo/serdes/type_info.h"

namespace eolo::serdes {

class DynamicDeserializer {
public:
  void registerSchema(const TypeInfo& type_info);
  [[nodiscard]] auto toJson(const std::string& type, std::span<const std::byte> data) -> std::string;

private:
  protobuf::DynamicDeserializer proto_deserializer_;
  std::unordered_map<std::string, TypeInfo::Serialization> type_to_serialization_;
};

}  // namespace eolo::serdes
