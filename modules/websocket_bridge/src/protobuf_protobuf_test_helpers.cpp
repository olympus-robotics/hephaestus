#include "hephaestus/websocket_bridge/protobuf_test_helpers.h"

namespace heph::ws_bridge {

RandomGenerators::RandomGenerators()
  : gen(std::random_device{}())
  , int32_dist(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max())
  , int64_dist(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max())
  , uint32_dist(std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max())
  , uint64_dist(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max())
  , float_dist(std::numeric_limits<float>::min(), std::numeric_limits<float>::max())
  , double_dist(std::numeric_limits<double>::min(), std::numeric_limits<double>::max()) {
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

std::vector<uint8_t>
generateRandomProtobufMessageFromSchema(const foxglove::ServiceRequestDefinition& service_definition) {
  // Decode the base64 string into binary schema.
  std::vector<unsigned char> schema_char_vector = foxglove::base64Decode(service_definition.schema);

  std::vector<std::byte> schema_bytes(schema_char_vector.size());
  std::transform(schema_char_vector.begin(), schema_char_vector.end(), schema_bytes.begin(),
                 [](unsigned char c) { return static_cast<std::byte>(c); });

  google::protobuf::SimpleDescriptorDatabase proto_db;
  loadSchema(schema_bytes, &proto_db);

  google::protobuf::DescriptorPool proto_pool(&proto_db);

  const google::protobuf::Descriptor* descriptor =
      proto_pool.FindMessageTypeByName(service_definition.schemaName);
  if (!descriptor) {
    fmt::print("Message type '{}' not found in schema\n", service_definition.schemaName);
    heph::ws_bridge::debugPrintSchema(schema_bytes);
    return {};
  }

  google::protobuf::DynamicMessageFactory proto_factory(&proto_pool);
  std::unique_ptr<google::protobuf::Message> message =
      std::unique_ptr<google::protobuf::Message>(proto_factory.GetPrototype(descriptor)->New());
  if (!message) {
    fmt::print("Failed to create message for type '{}'\n", service_definition.schemaName);
    heph::ws_bridge::debugPrintSchema(schema_bytes);
    return {};
  }

  // Fill the message with random values
  RandomGenerators generators;
  fillMessageWithRandomValues(message.get(), generators);

  // Serialize the message to a byte vector
  std::vector<uint8_t> buffer(message->ByteSizeLong());
  if (!message->SerializeToArray(buffer.data(), static_cast<int>(buffer.size()))) {
    fmt::print("Failed to serialize message\n");
    return {};
  }

  return buffer;
}

}  // namespace heph::ws_bridge
