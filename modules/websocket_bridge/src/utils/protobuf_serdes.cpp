//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/protobuf_serdes.h"

namespace heph::ws {

RandomGenerators::RandomGenerators()
  : gen(std::random_device{}())
  , int32_dist(-10, 10)
  , int64_dist(-10, 10)
  , uint32_dist(0, 10)
  , uint64_dist(0, 10)
  , float_dist(-10.0f, 10.0f)
  , double_dist(-10.0, 10.0) {
}

void fillRepeatedField(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                       RandomGenerators& generators, int depth) {
  auto reflection = message->GetReflection();
  int count = generators.int32_dist(generators.gen) % 10;
  for (int j = 0; j < count; ++j) {
    switch (field->type()) {
      case google::protobuf::FieldDescriptor::TYPE_BOOL:
        reflection->AddBool(message, field, generators.int32_dist(generators.gen) % 2);
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
  if (depth > 5) {
    return;  // Abort if recursion depth exceeds 5
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
          setRandomValue<std::string>(message, field, generators);
          break;
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

bool loadSchema(const std::vector<std::byte>& schema_bytes,
                google::protobuf::SimpleDescriptorDatabase* proto_db) {
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

bool saveSchemaToDatabase(const foxglove::Channel& channel_definition, ProtobufSchemaDatabase& schema_db) {
  if (channel_definition.schemaEncoding == "protobuf") {
    schema_db.channel_id_to_schema_name[channel_definition.id] = channel_definition.schemaName;

    // Decode the base64 string into binary schema.
    std::vector<unsigned char> schema_char_vector = foxglove::base64Decode(channel_definition.schema);

    std::vector<std::byte> schema_bytes(schema_char_vector.size());
    std::transform(schema_char_vector.begin(), schema_char_vector.end(), schema_bytes.begin(),
                   [](unsigned char c) { return static_cast<std::byte>(c); });

    return saveSchemaToDatabase(schema_bytes, schema_db);
  }
  return true;
}

bool saveSchemaToDatabase(const foxglove::Service& service_definition, ProtobufSchemaDatabase& schema_db) {
  if (!service_definition.request.has_value() || !service_definition.response.has_value()) {
    heph::log(heph::ERROR, "Service definition is missing request or response schema");
    return false;
  }

  schema_db.service_id_to_schema_names[service_definition.id] = { service_definition.request->schemaName,
                                                                  service_definition.response->schemaName };

  return saveSchemaToDatabase(service_definition.request.value(), schema_db) &&
         saveSchemaToDatabase(service_definition.response.value(), schema_db);
}

bool saveSchemaToDatabase(const foxglove::ServiceResponseDefinition& service_request_definition,
                          ProtobufSchemaDatabase& schema_db) {
  if (service_request_definition.schemaEncoding != "protobuf") {
    heph::log(heph::WARN, "Service request schema encoding is not protobuf. Will not add to database.");
    return false;
  }

  // Decode the base64 string into binary schema.
  std::vector<unsigned char> schema_char_vector = foxglove::base64Decode(service_request_definition.schema);

  std::vector<std::byte> schema_bytes(schema_char_vector.size());
  std::transform(schema_char_vector.begin(), schema_char_vector.end(), schema_bytes.begin(),
                 [](unsigned char c) { return static_cast<std::byte>(c); });

  return saveSchemaToDatabase(schema_bytes, schema_db);
}

bool saveSchemaToDatabase(const std::vector<std::byte>& schema_bytes, ProtobufSchemaDatabase& schema_db) {
  if (!loadSchema(schema_bytes, schema_db.proto_db.get())) {
    heph::log(heph::ERROR, "Failed to load schema into database");
    heph::ws::debugPrintSchema(schema_bytes);
    return false;
  }
  return true;
}

std::unique_ptr<google::protobuf::Message> retrieveResponseMessageFromDatabase(
    const foxglove::ServiceId service_id, const ProtobufSchemaDatabase& schema_db) {
  auto schema_names = retrieveSchemaNamesFromServiceId(service_id, schema_db);

  if (!schema_names.second.empty()) {
    auto response_message = retrieveMessageFromDatabase(schema_names.second, schema_db);
    return response_message;
  }

  heph::log(heph::ERROR, "Service id was not found in service to schema names map!", "service_id",
            service_id);
  return nullptr;
}

std::unique_ptr<google::protobuf::Message> retrieveRequestMessageFromDatabase(
    const foxglove::ServiceId service_id, const ProtobufSchemaDatabase& schema_db) {
  auto schema_names = retrieveSchemaNamesFromServiceId(service_id, schema_db);

  if (!schema_names.first.empty()) {
    auto request_message = retrieveMessageFromDatabase(schema_names.first, schema_db);
    return request_message;
  }

  heph::log(heph::ERROR, "Service id was not found in service to schema names map!", "service_id",
            service_id);
  return nullptr;
}

std::unique_ptr<google::protobuf::Message>
retrieveMessageFromDatabase(const std::string& schema_name, const ProtobufSchemaDatabase& schema_db) {
  const google::protobuf::Descriptor* descriptor = schema_db.proto_pool->FindMessageTypeByName(schema_name);
  if (!descriptor) {
    heph::log(heph::ERROR, "Message type not found in schema database", "schema_name", schema_name);
    return nullptr;
  }

  const google::protobuf::Message* prototype = schema_db.proto_factory->GetPrototype(descriptor);
  if (!prototype) {
    heph::log(heph::ERROR, "Failed to get prototype for message", "schema_name", schema_name);
    return nullptr;
  }

  return std::unique_ptr<google::protobuf::Message>(prototype->New());
}

std::pair<std::string, std::string> retrieveSchemaNamesFromServiceId(
    const foxglove::ServiceId service_id, const ProtobufSchemaDatabase& schema_db) {
  auto it = schema_db.service_id_to_schema_names.find(service_id);
  if (it != schema_db.service_id_to_schema_names.end()) {
    return it->second;
  }
  return {};
}

std::string retrieveSchemaNameFromChannelId(const foxglove::ChannelId channel_id,
                                            const ProtobufSchemaDatabase& schema_db) {
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

std::unique_ptr<google::protobuf::Message>
generateRandomMessageFromSchemaName(const std::string schema_name, ProtobufSchemaDatabase& schema_db) {
  // Retrieve the message from the database
  auto message = retrieveMessageFromDatabase(schema_name, schema_db);
  if (!message) {
    heph::log(heph::ERROR, "Failed to retrieve message from database");
    return {};
  }

  // Fill the message with random values
  RandomGenerators generators;
  fillMessageWithRandomValues(message.get(), generators);

  return message;
}

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
  fmt::print("Schema: \n'''\n{}\n'''\n", convertProtoMsgBytesToDebugString(schema));
}

void debugPrintMessage(const google::protobuf::Message& msg) {
  std::string json_string;
  const auto status = google::protobuf::util::MessageToJsonString(msg, &json_string);
  if (!status.ok()) {
    fmt::print("Failed to convert message to JSON: {}\n", status.ToString());
    return;
  }
  fmt::print("Message: \n'''\n{}\n'''\n", json_string);
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

  fmt::println("BINRARY ({} bytes)", length);

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

foxglove::ChannelWithoutId convertIpcTypeInfoToWsChannelInfo(const std::string& topic,
                                                             const serdes::TypeInfo& type_info) {
  foxglove::ChannelWithoutId channel_info;
  channel_info.topic = topic;
  channel_info.encoding = convertSerializationTypeToString(type_info.serialization);
  channel_info.schemaName = type_info.name;
  channel_info.schema = convertProtoBytesToFoxgloveBase64String(type_info.schema);
  channel_info.schemaEncoding = channel_info.encoding;
  return channel_info;
}

std::optional<serdes::TypeInfo>
convertWsChannelInfoToIpcTypeInfo(const foxglove::ClientAdvertisement& channel_info) {
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
  std::transform(encoding_upper.begin(), encoding_upper.end(), encoding_upper.begin(), ::toupper);

  auto schema_bytes_as_chars = foxglove::base64Decode(channel_info.schema.value());

  serdes::TypeInfo type_info;
  type_info.serialization = magic_enum::enum_cast<serdes::TypeInfo::Serialization>(encoding_upper).value();
  type_info.name = channel_info.schemaName;
  type_info.schema = std::vector<std::byte>(
      reinterpret_cast<std::byte*>(schema_bytes_as_chars.data()),
      reinterpret_cast<std::byte*>(schema_bytes_as_chars.data() + schema_bytes_as_chars.size()));

  fmt::print("'''\n{}\n'''", type_info.toJson());

  debugPrintSchema(type_info.schema);

  return type_info;
}

}  // namespace heph::ws
