#include "hephaestus/websocket_bridge/serialization_utils.h"

namespace heph::ws_bridge {

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