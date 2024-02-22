//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/serdes/protobuf/dynamic_deserializer.h"

#include <cstddef>

#include "eolo/base/exception.h"

namespace eolo::serdes::protobuf {
namespace {
void loadSchema(const eolo::serdes::TypeInfo& type_info,
                google::protobuf::SimpleDescriptorDatabase& proto_db) {
  google::protobuf::FileDescriptorSet fd_set;
  auto res = fd_set.ParseFromArray(type_info.schema.data(), static_cast<int>(type_info.schema.size()));
  throwExceptionIf<InvalidDataException>(!res,
                                         std::format("failed to parse schema for type: {}", type_info.name));

  google::protobuf::FileDescriptorProto unused;
  for (int i = 0; i < fd_set.file_size(); ++i) {
    const auto& file = fd_set.file(i);
    if (!proto_db.FindFileByName(file.name(), &unused)) {
      res = proto_db.Add(file);
      throwExceptionIf<InvalidDataException>(
          !res, std::format("failed to add definition to proto DB: {}", file.name()));
    }
  }
}
}  // namespace

void DynamicDeserializer::registerSchema(const TypeInfo& type_info) {
  const auto* descriptor = proto_pool_.FindMessageTypeByName(type_info.name);
  if (descriptor != nullptr) {
    return;
  }

  loadSchema(type_info, proto_db_);
}

auto DynamicDeserializer::toJson(const std::string& type, std::span<const std::byte> data) -> std::string {
  const auto* descriptor = proto_pool_.FindMessageTypeByName(type);
  throwExceptionIf<InvalidDataException>(
      descriptor == nullptr,
      std::format("schema for type {} was not registered, cannot deserialize data", type));

  auto* message = proto_factory_.GetPrototype(descriptor)->New();
  const auto res = message->ParseFromArray(data.data(), static_cast<int>(data.size()));
  throwExceptionIf<InvalidDataException>(!res, std::format("failed to parse message of type {}", type));

  std::string msg_json;
  google::protobuf::util::JsonPrintOptions options;
  options.always_print_primitive_fields = true;
  auto status = google::protobuf::util::MessageToJsonString(*message, &msg_json);
  throwExceptionIf<InvalidDataException>(
      !status.ok(), std::format("failed to convert proto message to json: {}", status.message()));

  return msg_json;
}

}  // namespace eolo::serdes::protobuf
