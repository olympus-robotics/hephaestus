//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include "hephaestus/serdes/protobuf/dynamic_deserializer.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::serdes {

class DynamicDeserializer {
public:
  void registerSchema(const TypeInfo& type_info);
  [[nodiscard]] auto toJson(const std::string& type, std::span<const std::byte> data) -> std::string;
  [[nodiscard]] auto toText(const std::string& type, std::span<const std::byte> data) -> std::string;

private:
  protobuf::DynamicDeserializer proto_deserializer_;
  std::unordered_map<std::string, TypeInfo::Serialization> type_to_serialization_;
};

}  // namespace heph::serdes
