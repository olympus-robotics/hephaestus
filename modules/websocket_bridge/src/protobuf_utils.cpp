#include "hephaestus/websocket_bridge/protobuf_utils.h"

namespace heph::ws_bridge {

void fillRepeatedField(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                       std::mt19937& gen, std::uniform_int_distribution<int32_t>& int32_dist,
                       std::uniform_int_distribution<int64_t>& int64_dist,
                       std::uniform_int_distribution<uint32_t>& uint32_dist,
                       std::uniform_int_distribution<uint64_t>& uint64_dist,
                       std::uniform_real_distribution<float>& float_dist,
                       std::uniform_real_distribution<double>& double_dist, int depth) {
  auto reflection = message->GetReflection();
  int count = int32_dist(gen) % 10;
  for (int j = 0; j < count; ++j) {
    switch (field->type()) {
      case google::protobuf::FieldDescriptor::TYPE_BOOL:
        reflection->AddBool(message, field, int32_dist(gen) % 2);
        break;
      case google::protobuf::FieldDescriptor::TYPE_INT32:
        reflection->AddInt32(message, field, int32_dist(gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_INT64:
        reflection->AddInt64(message, field, int64_dist(gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_UINT32:
        reflection->AddUInt32(message, field, uint32_dist(gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_UINT64:
        reflection->AddUInt64(message, field, uint64_dist(gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_FLOAT:
        reflection->AddFloat(message, field, float_dist(gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
        reflection->AddDouble(message, field, double_dist(gen));
        break;
      case google::protobuf::FieldDescriptor::TYPE_STRING:
        reflection->AddString(message, field, "random_string");
        break;
      case google::protobuf::FieldDescriptor::TYPE_BYTES:
        reflection->AddString(message, field, "random_bytes");
        break;
      case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
        fillMessageWithRandomValues(reflection->AddMessage(message, field), depth + 1);
        break;
      default:
        break;
    }
  }
}

void fillMessageWithRandomValues(google::protobuf::Message* message, int depth) {
  if (depth > 5) {
    return;  // Abort if recursion depth exceeds 5
  }

  const google::protobuf::Reflection* reflection = message->GetReflection();
  const google::protobuf::Descriptor* msg_descriptor = message->GetDescriptor();

  std::random_device rd;
  std::mt19937 gen(rd());

  std::uniform_int_distribution<int32_t> int32_dist;
  std::uniform_int_distribution<int64_t> int64_dist;
  std::uniform_int_distribution<uint32_t> uint32_dist;
  std::uniform_int_distribution<uint64_t> uint64_dist(0, UINT64_MAX);
  std::uniform_real_distribution<float> float_dist(0.0f, 1.0f);
  std::uniform_real_distribution<double> double_dist(0.0, 1.0);

  for (int i = 0; i < msg_descriptor->field_count(); ++i) {
    const google::protobuf::FieldDescriptor* field = msg_descriptor->field(i);
    if (field->is_repeated()) {
      fillRepeatedField(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist, float_dist,
                        double_dist, depth);
    } else {
      switch (field->type()) {
        case google::protobuf::FieldDescriptor::TYPE_BOOL:
          setRandomValue<bool>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                               float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_INT32:
          setRandomValue<int32_t>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                  float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_INT64:
          setRandomValue<int64_t>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                  float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_UINT32:
          setRandomValue<uint32_t>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                   float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_UINT64:
          setRandomValue<uint64_t>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                   float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_FLOAT:
          setRandomValue<float>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
          setRandomValue<double>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                 float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_STRING:
          setRandomValue<std::string>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                      float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_BYTES:
          setRandomValue<std::string>(message, field, gen, int32_dist, int64_dist, uint32_dist, uint64_dist,
                                      float_dist, double_dist);
          break;
        case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
          fillMessageWithRandomValues(reflection->MutableMessage(message, field), depth + 1);
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

  //   fmt::print("Descriptor:\n```\n{}\n```\n", descriptor->DebugString());

  google::protobuf::DynamicMessageFactory proto_factory(&proto_pool);
  std::unique_ptr<google::protobuf::Message> message =
      std::unique_ptr<google::protobuf::Message>(proto_factory.GetPrototype(descriptor)->New());
  if (!message) {
    fmt::print("Failed to create message for type '{}'\n", service_definition.schemaName);
    heph::ws_bridge::debugPrintSchema(schema_bytes);
    return {};
  }

  const google::protobuf::FieldDescriptor* position_descriptor = descriptor->FindFieldByName("position");
  const google::protobuf::FieldDescriptor* orientation_descriptor =
      descriptor->FindFieldByName("orientation");

  if (position_descriptor && orientation_descriptor) {
    const google::protobuf::Reflection* reflection = message->GetReflection();
    google::protobuf::Message* position_message =
        reflection->MutableMessage(message.get(), position_descriptor);
    google::protobuf::Message* orientation_message =
        reflection->MutableMessage(message.get(), orientation_descriptor);

    const google::protobuf::Reflection* position_reflection = position_message->GetReflection();
    const google::protobuf::Reflection* orientation_reflection = orientation_message->GetReflection();

    const google::protobuf::Descriptor* position_msg_descriptor = position_message->GetDescriptor();
    const google::protobuf::Descriptor* orientation_msg_descriptor = orientation_message->GetDescriptor();

    const google::protobuf::FieldDescriptor* x_field = position_msg_descriptor->FindFieldByName("x");
    const google::protobuf::FieldDescriptor* y_field = position_msg_descriptor->FindFieldByName("y");
    const google::protobuf::FieldDescriptor* z_field = position_msg_descriptor->FindFieldByName("z");

    const google::protobuf::FieldDescriptor* w_field = orientation_msg_descriptor->FindFieldByName("w");
    const google::protobuf::FieldDescriptor* ox_field = orientation_msg_descriptor->FindFieldByName("x");
    const google::protobuf::FieldDescriptor* oy_field = orientation_msg_descriptor->FindFieldByName("y");
    const google::protobuf::FieldDescriptor* oz_field = orientation_msg_descriptor->FindFieldByName("z");

    if (x_field && y_field && z_field && w_field && ox_field && oy_field && oz_field) {
      position_reflection->SetDouble(position_message, x_field, 1.0);
      position_reflection->SetDouble(position_message, y_field, 2.0);
      position_reflection->SetDouble(position_message, z_field, 3.0);

      orientation_reflection->SetDouble(orientation_message, w_field, 1.0);
      orientation_reflection->SetDouble(orientation_message, ox_field, 0.0);
      orientation_reflection->SetDouble(orientation_message, oy_field, 0.0);
      orientation_reflection->SetDouble(orientation_message, oz_field, 0.0);
    }
  }

  // Fill the message with random values
  fillMessageWithRandomValues(message.get());

  // Print the message for debugging
  //   fmt::print("Generated message:\n```\n{}\n```\n", message->DebugString());
  //   fmt::print("Generated message size:\n```\n{}\n```\n", message->ByteSizeLong());

  // Serialize the message to a byte vector
  std::vector<uint8_t> buffer(message->ByteSizeLong());
  if (!message->SerializeToArray(buffer.data(), static_cast<int>(buffer.size()))) {
    fmt::print("Failed to serialize message\n");
    return {};
  }

  return buffer;
}

}  // namespace heph::ws_bridge