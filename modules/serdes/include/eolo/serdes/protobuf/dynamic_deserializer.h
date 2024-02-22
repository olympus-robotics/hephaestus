//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>

#include "eolo/serdes/type_info.h"

namespace eolo::serdes::protobuf {

class DynamicDeserializer {
public:
  void registerSchema(const TypeInfo& type_info);
  [[nodiscard]] auto toJson(const std::string& type, std::span<const std::byte> data) -> std::string;

private:
  google::protobuf::SimpleDescriptorDatabase proto_db_;
  google::protobuf::DescriptorPool proto_pool_{ &proto_db_ };
  google::protobuf::DynamicMessageFactory proto_factory_{ &proto_pool_ };
};

}  // namespace eolo::serdes::protobuf
