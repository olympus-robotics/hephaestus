#include "hephaestus/websocket_bridge/protobuf_utils.h"

namespace heph::ws_bridge {

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

bool saveSchemaToDatabase(const foxglove::Service& service_definition, ProtobufSchemaDatabase& schema_db) {
  if (!service_definition.request.has_value() || !service_definition.response.has_value()) {
    fmt::print("Service definition is missing request or response schema\n");
    return false;
  }

  schema_db.service_id_to_schema_names[service_definition.id] = { service_definition.request->schemaName,
                                                                  service_definition.response->schemaName };

  return saveSchemaToDatabase(service_definition.request.value(), schema_db) &&
         saveSchemaToDatabase(service_definition.response.value(), schema_db);
}

bool saveSchemaToDatabase(const foxglove::ServiceResponseDefinition& service_request_definition,
                          ProtobufSchemaDatabase& schema_db) {
  // Decode the base64 string into binary schema.
  std::vector<unsigned char> schema_char_vector = foxglove::base64Decode(service_request_definition.schema);

  std::vector<std::byte> schema_bytes(schema_char_vector.size());
  std::transform(schema_char_vector.begin(), schema_char_vector.end(), schema_bytes.begin(),
                 [](unsigned char c) { return static_cast<std::byte>(c); });

  return saveSchemaToDatabase(schema_bytes, schema_db);
}

bool saveSchemaToDatabase(const std::vector<std::byte>& schema_bytes, ProtobufSchemaDatabase& schema_db) {
  if (!loadSchema(schema_bytes, schema_db.proto_db.get())) {
    fmt::print("Failed to load schema into database\n");
    heph::ws_bridge::debugPrintSchema(schema_bytes);
    return false;
  }
  return true;
}

std::unique_ptr<google::protobuf::Message>
retreiveMessageFromDatabase(const std::string& schema_name, const ProtobufSchemaDatabase& schema_db) {
  const google::protobuf::Descriptor* descriptor = schema_db.proto_pool->FindMessageTypeByName(schema_name);
  if (!descriptor) {
    fmt::print("Message type '{}' not found in schema database\n", schema_name);
    return nullptr;
  }

  const google::protobuf::Message* prototype = schema_db.proto_factory->GetPrototype(descriptor);
  if (!prototype) {
    fmt::print("Failed to get prototype for message type '{}'\n", schema_name);
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
  auto message = retreiveMessageFromDatabase(schema_name, schema_db);
  if (!message) {
    fmt::print("Failed to retrieve message from database\n");
    return {};
  }

  // Fill the message with random values
  RandomGenerators generators;
  fillMessageWithRandomValues(message.get(), generators);

  return message;
}

}  // namespace heph::ws_bridge
