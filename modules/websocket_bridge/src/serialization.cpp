#include "hephaestus/websocket_bridge/serialization.h"

#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/serialization.hpp>

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

void printBinary(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    fmt::print("No data to print.\n");
    return;
  }

  std::stringstream ss;
  for (size_t i = 0; i < length; ++i) {
    for (int bit = 7; bit >= 0; --bit) {
      ss << ((data[i] >> bit) & 1);
      if (bit == 4) {
        ss << " | ";
      }
    }
    if ((i + 1) % 4 == 0) {
      uint32_t uint32_value = foxglove::ReadUint32LE(data + i - 3);
      ss << " ==> " << uint32_value << "\n";
    } else if (i < length - 1) {
      ss << " || ";
    }
  }
  if (length % 4 != 0) {
    ss << "\n";
  }

  fmt::print("{}", ss.str());
}

std::string getTimestampString() {
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::tm now_tm;
  localtime_r(&now_time_t, &now_tm);

  std::ostringstream oss;
  oss << "" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3)
      << now_ms.count();

  return oss.str();
}

}  // namespace heph::ws_bridge