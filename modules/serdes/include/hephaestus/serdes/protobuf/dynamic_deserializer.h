//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>

#include "hephaestus/serdes/type_info.h"

namespace heph::serdes::protobuf {

class DynamicDeserializer {
public:
  void registerSchema(const TypeInfo& type_info);
  [[nodiscard]] auto toJson(const std::string& type, std::span<const std::byte> data) -> std::string;
  [[nodiscard]] auto toText(const std::string& type, std::span<const std::byte> data) -> std::string;

private:
  [[nodiscard]] auto getMessage(const std::string& type,
                                std::span<const std::byte> data) -> google::protobuf::Message*;

private:
  google::protobuf::SimpleDescriptorDatabase proto_db_;
  google::protobuf::DescriptorPool proto_pool_{ &proto_db_ };
  google::protobuf::DynamicMessageFactory proto_factory_{ &proto_pool_ };
};

}  // namespace heph::serdes::protobuf
