//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/serdes/protobuf/dynamic_deserializer.h"

#include <cstddef>
#include <span>
#include <string>

#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::serdes::protobuf {
namespace {
void loadSchema(const heph::serdes::TypeInfo& type_info,
                google::protobuf::SimpleDescriptorDatabase& proto_db) {
  google::protobuf::FileDescriptorSet fd_set;
  auto res = fd_set.ParseFromArray(type_info.schema.data(), static_cast<int>(type_info.schema.size()));
  HEPH_PANIC_IF(!res, "failed to parse schema for type: {}", type_info.name);

  google::protobuf::FileDescriptorProto unused;
  for (int i = 0; i < fd_set.file_size(); ++i) {
    const auto& file = fd_set.file(i);
    if (!proto_db.FindFileByName(file.name(), &unused)) {
      res = proto_db.Add(file);
      HEPH_PANIC_IF(!res, "failed to add definition to proto DB: {}", file.name());
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
  auto* message = getMessage(type, data);

  std::string msg_json;
  google::protobuf::util::JsonPrintOptions options;
  options.always_print_fields_with_no_presence = true;
  options.add_whitespace = true;
  options.unquote_int64_if_possible = true;
  const auto status = google::protobuf::util::MessageToJsonString(*message, &msg_json, options);
  HEPH_PANIC_IF(!status.ok(), "failed to convert proto message of type {} to json: {}", type,
                status.message());

  return msg_json;
}

auto DynamicDeserializer::toText(const std::string& type, std::span<const std::byte> data) -> std::string {
  auto* message = getMessage(type, data);
  std::string msg_text;
  auto printer = google::protobuf::TextFormat::Printer();
  printer.SetUseShortRepeatedPrimitives(true);
  const auto success = printer.PrintToString(*message, &msg_text);
  HEPH_PANIC_IF(!success, "failed to convert proto message of type {} to text", type);

  return msg_text;
}

auto DynamicDeserializer::getMessage(const std::string& type, std::span<const std::byte> data)
    -> google::protobuf::Message* {
  const auto* descriptor = proto_pool_.FindMessageTypeByName(type);
  HEPH_PANIC_IF(descriptor == nullptr, "schema for type {} was not registered, cannot deserialize data",
                type);

  auto* message = proto_factory_.GetPrototype(descriptor)->New();
  const auto res = message->ParseFromArray(data.data(), static_cast<int>(data.size()));
  HEPH_PANIC_IF(!res, "failed to parse message of type {}", type);

  return message;
}

}  // namespace heph::serdes::protobuf
