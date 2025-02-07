#include "hephaestus/websocket_bridge/serialization.h"

namespace heph::ws_bridge {

// void addTimestampFieldToFirstFileDescriptor(google::protobuf::FileDescriptorSet& fds) {
//   if (fds.file_size() == 0) {
//     std::cerr << "FileDescriptorSet is empty" << std::endl;
//     return;
//   }

//   auto* file_descriptor_proto = fds.mutable_file(0);
//   if (!file_descriptor_proto) {
//     std::cerr << "Failed to get the first FileDescriptorProto" << std::endl;
//     return;
//   }

//   auto* message_type = file_descriptor_proto->mutable_message_type(0);
//   if (!message_type) {
//     std::cerr << "Failed to get the first DescriptorProto" << std::endl;
//     return;
//   }

//   auto* field = message_type->add_field();
//   field->set_name("timestamp");
//   field->set_number(message_type->field_size());
//   field->set_label(google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL);
//   field->set_type(google::protobuf::FieldDescriptorProto::TYPE_INT64);
// }

std::string convertProtoMsgBytesToDebugString(const std::vector<std::byte>& schema) {
  google::protobuf::FileDescriptorSet fds;
  if (!fds.ParseFromArray(schema.data(), static_cast<int>(schema.size()))) {
    std::cerr << "Failed to parse schema bytes" << std::endl;
    return {};
  }
  std::cout << fds.DebugString() << std::endl;
  return fds.DebugString();
}

void debugPrintSchema(const std::vector<std::byte>& schema) {
  fmt::print("Schema: {}\n", convertProtoMsgBytesToDebugString(schema));
}

std::string convertProtoBytesToFoxgloveBase64String(const std::vector<std::byte>& data) {
  std::string_view data_view(reinterpret_cast<const char*>(data.data()), data.size());
  return foxglove::base64Encode(data_view);
}

std::string convertSerializationTypeToString(const serdes::TypeInfo::Serialization& serialization) {
  auto schema_type = std::string{ magic_enum::enum_name(serialization) };
  absl::AsciiStrToLower(&schema_type);
  return schema_type;
}

}  // namespace heph::ws_bridge