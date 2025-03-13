//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/websocket_bridge/utils/protobuf_serdes.h"

#include <algorithm>
#include <bit>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <absl/log/log.h>
#include <absl/strings/ascii.h>
#include <fmt/base.h>
#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/serialization.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <hephaestus/serdes/type_info.h>
#include <hephaestus/telemetry/log.h>
#include <magic_enum.hpp>

namespace heph::ws {

namespace {
auto loadSchema(const std::vector<std::byte>& schema_bytes,
                google::protobuf::SimpleDescriptorDatabase* proto_db) -> bool {
  if (schema_bytes.empty()) {
    heph::log(heph::ERROR, "Cannot loadSchema -> Schema bytes are empty");
    return false;
  }

  google::protobuf::FileDescriptorSet descriptor_set;
  if (!descriptor_set.ParseFromArray(static_cast<const void*>(schema_bytes.data()),
                                     static_cast<int>(schema_bytes.size()))) {
    return false;
  }
  google::protobuf::FileDescriptorProto unused;
  for (int i = 0; i < descriptor_set.file_size(); ++i) {
    const auto& file = descriptor_set.file(i);
    if (!proto_db->FindFileByName(file.name(), &unused)) {
      if (!proto_db->Add(file)) {
        return false;
      }
    }
  }
  return true;
}

auto saveSchemaToDatabase(const std::vector<std::byte>& schema_bytes, ProtobufSchemaDatabase& schema_db)
    -> bool {
  if (!loadSchema(schema_bytes, schema_db.proto_db.get())) {
    heph::log(heph::ERROR, "Failed to load schema into database");
    heph::ws::debugPrintSchema(schema_bytes);
    return false;
  }
  return true;
}
}  // namespace

RandomGenerators::RandomGenerators(int min, int max)
  : gen(std::random_device{}())
  , int32_dist(min, max)
  , int64_dist(min, max)
  , uint32_dist(static_cast<uint32_t>(std::max(0, min)), static_cast<uint32_t>(std::max(0, max)))
  , uint64_dist(static_cast<uint64_t>(std::max(0, min)), static_cast<uint64_t>(std::max(0, max)))
  , float_dist(static_cast<float>(min), static_cast<float>(max))
  , double_dist(static_cast<double>(min), static_cast<double>(max)) {
}

void fillRepeatedField(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                       RandomGenerators& generators, int depth) {
  const auto* reflection = message->GetReflection();
  static constexpr int MAX_REPEATED_FIELD_COUNT = 10;
  const int count = generators.int32_dist(generators.gen) % MAX_REPEATED_FIELD_COUNT;
  for (int j = 0; j < count; ++j) {
    switch (field->type()) {
      case google::protobuf::FieldDescriptor::TYPE_BOOL:
        reflection->AddBool(message, field, (generators.int32_dist(generators.gen) % 2) != 0);
        break;
      case google::protobuf::FieldDescriptor::TYPE_INT32:
        reflection->AddInt32(message, field, generators.int32_dist(generators.gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_INT64:
        reflection->AddInt64(message, field, generators.int64_dist(generators.gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_UINT32:
        reflection->AddUInt32(message, field, generators.uint32_dist(generators.gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_UINT64:
        reflection->AddUInt64(message, field, generators.uint64_dist(generators.gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        reflection->AddFloat(message, field, generators.float_dist(generators.gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        reflection->AddDouble(message, field, generators.double_dist(generators.gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_STRING:
        reflection->AddString(message, field, "random_string");
        break;
      case google::protobuf::FieldDescriptor::TYPE_BYTES:
        reflection->AddString(message, field, "random_bytes");
        break;
      case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
        fillMessageWithRandomValues(reflection->AddMessage(message, field), generators, depth + 1);
        break;
      default:
        break;
    }
  }
}

void fillMessageWithRandomValues(google::protobuf::Message* message, RandomGenerators& generators,
                                 int depth) {
  static constexpr int MAX_RECURSION_DEPTH = 5;
  if (depth > MAX_RECURSION_DEPTH) {
    return;  // Abort if recursion depth exceeds MAX_RECURSION_DEPTH
  }

  const google::protobuf::Reflection* reflection = message->GetReflection();
  const google::protobuf::Descriptor* msg_descriptor = message->GetDescriptor();

  for (int i = 0; i < msg_descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = msg_descriptor->field(i);
    if (field->is_repeated()) {
      fillRepeatedField(message, field, generators, depth);
    } else {
      switch (field->type()) {
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
          setRandomValue<bool>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
          setRandomValue<int32_t>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
          setRandomValue<int64_t>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
          setRandomValue<uint32_t>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
          setRandomValue<uint64_t>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
          setRandomValue<float>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
          setRandomValue<double>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
        // Fallthrough intended.
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
          setRandomValue<std::string>(message, field, generators);
          break;
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
          fillMessageWithRandomValues(reflection->MutableMessage(message, field), generators, depth + 1);
          break;
        default:
          break;  // Handle other types as needed
      }
    }
  }
}

auto saveSchemaToDatabase(const foxglove::Channel& channel_definition, ProtobufSchemaDatabase& schema_db)
    -> bool {
  if (channel_definition.schemaEncoding == "protobuf") {
    schema_db.channel_id_to_schema_name[channel_definition.id] = channel_definition.schemaName;

    // Decode the base64 string into binary schema.
    std::vector<unsigned char> schema_char_vector = foxglove::base64Decode(channel_definition.schema);

    std::vector<std::byte> schema_bytes(schema_char_vector.size());
    std::ranges::transform(schema_char_vector, schema_bytes.begin(),
                           [](unsigned char c) { return static_cast<std::byte>(c); });

    return saveSchemaToDatabase(schema_bytes, schema_db);
  }
  return true;
}

auto saveSchemaToDatabase(const foxglove::Service& service_definition, ProtobufSchemaDatabase& schema_db)
    -> bool {
  if (!service_definition.request.has_value() || !service_definition.response.has_value()) {
    heph::log(heph::ERROR, "Service definition is missing request or response schema");
    return false;
  }

  schema_db.service_id_to_schema_names[service_definition.id] = { service_definition.request->schemaName,
                                                                  service_definition.response->schemaName };

  return saveSchemaToDatabase(service_definition.request.value(), schema_db) &&
         saveSchemaToDatabase(service_definition.response.value(), schema_db);
}

auto saveSchemaToDatabase(const foxglove::ServiceResponseDefinition& service_request_definition,
                          ProtobufSchemaDatabase& schema_db) -> bool {
  if (service_request_definition.schemaEncoding != "protobuf") {
    heph::log(heph::WARN, "Service request schema encoding is not protobuf. Will not add to database.");
    return false;
  }

  // Decode the base64 string into binary schema.
  std::vector<unsigned char> schema_char_vector = foxglove::base64Decode(service_request_definition.schema);

  std::vector<std::byte> schema_bytes(schema_char_vector.size());
  std::ranges::transform(schema_char_vector, schema_bytes.begin(),
                         [](unsigned char c) { return static_cast<std::byte>(c); });

  return saveSchemaToDatabase(schema_bytes, schema_db);
}

auto retrieveResponseMessageFromDatabase(foxglove::ServiceId service_id,
                                         const ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message> {
  auto schema_names = retrieveSchemaNamesFromServiceId(service_id, schema_db);

  if (!schema_names.second.empty()) {
    auto response_message = retrieveMessageFromDatabase(schema_names.second, schema_db);
    return response_message;
  }

  heph::log(heph::ERROR, "Service id was not found in service to schema names map!", "service_id",
            service_id);
  return nullptr;
}

auto retrieveRequestMessageFromDatabase(foxglove::ServiceId service_id,
                                        const ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message> {
  auto schema_names = retrieveSchemaNamesFromServiceId(service_id, schema_db);

  if (!schema_names.first.empty()) {
    auto request_message = retrieveMessageFromDatabase(schema_names.first, schema_db);
    return request_message;
  }

  heph::log(heph::ERROR, "Service id was not found in service to schema names map!", "service_id",
            service_id);
  return nullptr;
}

auto retrieveMessageFromDatabase(const std::string& schema_name, const ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message> {
  const google::protobuf::Descriptor* descriptor = schema_db.proto_pool->FindMessageTypeByName(schema_name);
  if (descriptor == nullptr) {
    heph::log(heph::ERROR, "Message type not found in schema database", "schema_name", schema_name);
    return nullptr;
  }

  const google::protobuf::Message* prototype = schema_db.proto_factory->GetPrototype(descriptor);
  if (prototype == nullptr) {
    heph::log(heph::ERROR, "Failed to get prototype for message", "schema_name", schema_name);
    return nullptr;
  }

  return std::unique_ptr<google::protobuf::Message>(prototype->New());
}

auto retrieveSchemaNamesFromServiceId(const foxglove::ServiceId service_id,
                                      const ProtobufSchemaDatabase& schema_db)
    -> std::pair<std::string, std::string> {
  auto it = schema_db.service_id_to_schema_names.find(service_id);
  if (it != schema_db.service_id_to_schema_names.end()) {
    return it->second;
  }
  return {};
}

auto retrieveSchemaNameFromChannelId(foxglove::ChannelId channel_id, const ProtobufSchemaDatabase& schema_db)
    -> std::string {
  auto it = schema_db.channel_id_to_schema_name.find(channel_id);
  if (it != schema_db.channel_id_to_schema_name.end()) {
    return it->second;
  }
  return {};
}

// Ensure ProtobufSchemaDatabase member variables are properly initialized
ProtobufSchemaDatabase::ProtobufSchemaDatabase()
  : proto_db(std::make_unique<google::protobuf::SimpleDescriptorDatabase>())
  , proto_pool(std::make_unique<google::protobuf::DescriptorPool>(proto_db.get()))
  , proto_factory(std::make_unique<google::protobuf::DynamicMessageFactory>(proto_pool.get())) {
}

auto generateRandomMessageFromSchemaName(const std::string& schema_name, ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message> {
  // Retrieve the message from the database
  auto message = retrieveMessageFromDatabase(schema_name, schema_db);
  if (!message) {
    heph::log(heph::ERROR, "Failed to retrieve message from database");
    return {};
  }

  // Fill the message with random values
  static constexpr int RANDOM_GENERATOR_MIN = -10;
  static constexpr int RANDOM_GENERATOR_MAX = 10;
  RandomGenerators generators(RANDOM_GENERATOR_MIN, RANDOM_GENERATOR_MAX);
  fillMessageWithRandomValues(message.get(), generators);

  return message;
}

namespace {
auto convertProtoMsgBytesToDebugString(const std::vector<std::byte>& schema) -> std::string {
  google::protobuf::FileDescriptorSet fds;
  if (!fds.ParseFromArray(schema.data(), static_cast<int>(schema.size()))) {
    heph::log(heph::ERROR, "Failed to parse schema bytes");
    return {};
  }
  return fds.DebugString();
}
}  // namespace

void debugPrintSchema(const std::vector<std::byte>& schema) {
  fmt::println("Schema: \n'''\n{}\n'''", convertProtoMsgBytesToDebugString(schema));
}

void debugPrintMessage(const google::protobuf::Message& message) {
  std::string json_string;
  const auto status = google::protobuf::util::MessageToJsonString(message, &json_string);
  if (!status.ok()) {
    fmt::println("Failed to convert message to JSON: {}", status.ToString());
    return;
  }
  fmt::println("Message: \n'''\n{}\n'''", json_string);
}

auto convertProtoBytesToFoxgloveBase64String(const std::vector<std::byte>& data) -> std::string {
  const std::string_view data_view{ std::bit_cast<const char*>(data.data()), data.size() };
  return foxglove::base64Encode(data_view);
}

auto convertSerializationTypeToString(const serdes::TypeInfo::Serialization& serialization) -> std::string {
  auto schema_type = std::string{ magic_enum::enum_name(serialization) };
  absl::AsciiStrToLower(&schema_type);
  return schema_type;
}

// NOLINTBEGIN
void printBinary(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    fmt::println("No data to print.");
    return;
  }

  fmt::println("BINRARY ({} bytes)", length);

  std::stringstream ss;
  for (size_t i = 0; i < length; ++i) {
    for (int bit = 7; bit >= 0; --bit) {
      // Cast to unsigned to avoid sign extensions
      ss << ((static_cast<unsigned int>(data[i]) >> bit) & 1U);
      if (bit == 4) {
        ss << " | ";
      }
    }
    // Use size_t for modulus to avoid mixed signed/unsigned
    if ((i + 1) % static_cast<size_t>(4) == 0) {
      // Also avoid mixed indexing
      uint32_t uint32_value = foxglove::ReadUint32LE(data + (i - 3));
      ss << " ==> " << uint32_value << "\n";
    } else if ((i + 1) < length) {
      ss << " || ";
    }
  }
  if ((length % static_cast<size_t>(4)) != 0U) {
    ss << "\n";
  }

  fmt::println("{}", ss.str());
}
// NOLINTEND

auto getTimestampString() -> std::string {
  auto now = std::chrono::system_clock::now();
  auto now_time_t = std::chrono::system_clock::to_time_t(now);
  static constexpr int MILLISECONDS_IN_SECOND = 1000;
  auto now_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % MILLISECONDS_IN_SECOND;

  std::tm now_tm{};
  if (localtime_r(&now_time_t, &now_tm) == nullptr) {
    fmt::println("Failed to convert time to local time.");
    return {};
  }

  std::ostringstream oss;
  oss << "" << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3)
      << now_ms.count();

  return oss.str();
}

auto convertIpcTypeInfoToWsChannelInfo(const std::string& topic, const serdes::TypeInfo& type_info)
    -> foxglove::ChannelWithoutId {
  foxglove::ChannelWithoutId channel_info;
  channel_info.topic = topic;
  channel_info.encoding = convertSerializationTypeToString(type_info.serialization);
  channel_info.schemaName = type_info.name;
  channel_info.schema = convertProtoBytesToFoxgloveBase64String(type_info.schema);
  channel_info.schemaEncoding = channel_info.encoding;
  return channel_info;
}

auto convertWsChannelInfoToIpcTypeInfo(const foxglove::ClientAdvertisement& channel_info)
    -> std::optional<serdes::TypeInfo> {
  // Check if the channel info is valid. Since this is completely at the mercy
  // of whoever writes the client, we need to check this thoroughly.
  if (!channel_info.schemaEncoding.has_value() || !channel_info.schema.has_value()) {
    heph::log(heph::ERROR, "Schema or schema encoding is not set in client advertisement!");
    return std::nullopt;
  }
  if (channel_info.schemaEncoding->empty()) {
    heph::log(heph::ERROR, "Schema encoding is empty!");
    return std::nullopt;
  }
  if (channel_info.schemaEncoding.value() != "protobuf") {
    heph::log(heph::ERROR, "Schema encoding is not protobuf!");
    return std::nullopt;
  }
  if (channel_info.encoding.empty()) {
    heph::log(heph::ERROR, "Encoding is empty!");
    return std::nullopt;
  }
  if (channel_info.encoding != "protobuf") {
    heph::log(heph::ERROR, "Encoding is not protobuf!");
    return std::nullopt;
  }
  if (channel_info.schemaName.empty()) {
    heph::log(heph::ERROR, "Schema name is empty!");
    return std::nullopt;
  }
  if (channel_info.schema->empty()) {
    heph::log(heph::ERROR, "Schema is empty!");
    return std::nullopt;
  }

  std::string encoding_upper = channel_info.encoding;
  std::ranges::transform(encoding_upper, encoding_upper.begin(), ::toupper);

  auto schema_bytes_as_chars = foxglove::base64Decode(channel_info.schema.value());
  std::vector<std::byte> schema_bytes(schema_bytes_as_chars.size());
  std::ranges::transform(schema_bytes_as_chars, schema_bytes.begin(),
                         [](unsigned char c) { return static_cast<std::byte>(c); });

  serdes::TypeInfo type_info;
  if (auto maybe_enum = magic_enum::enum_cast<serdes::TypeInfo::Serialization>(encoding_upper)) {
    type_info.serialization = *maybe_enum;
  } else {
    heph::log(heph::ERROR, "Failed to cast encoding to known serialization type", "encoding", encoding_upper);
    return std::nullopt;
  }

  type_info.name = channel_info.schemaName;
  type_info.schema = schema_bytes;

  return type_info;
}

}  // namespace heph::ws
