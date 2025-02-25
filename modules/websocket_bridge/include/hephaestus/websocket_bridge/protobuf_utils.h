#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <random>
#include <regex>
#include <thread>
#include <unordered_map>

#include <fmt/core.h>
#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/common.hpp>
#include <foxglove/websocket/serialization.hpp>
#include <foxglove/websocket/websocket_client.hpp>
#include <foxglove/websocket/websocket_notls.hpp>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <hephaestus/websocket_bridge/serialization.h>
#include <nlohmann/json.hpp>

namespace heph::ws_bridge {

template <typename T>
void setRandomValue(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                    std::mt19937& gen, std::uniform_int_distribution<int32_t>& int32_dist,
                    std::uniform_int_distribution<int64_t>& int64_dist,
                    std::uniform_int_distribution<uint32_t>& uint32_dist,
                    std::uniform_int_distribution<uint64_t>& uint64_dist,
                    std::uniform_real_distribution<float>& float_dist,
                    std::uniform_real_distribution<double>& double_dist);

void fillRepeatedField(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                       std::mt19937& gen, std::uniform_int_distribution<int32_t>& int32_dist,
                       std::uniform_int_distribution<int64_t>& int64_dist,
                       std::uniform_int_distribution<uint32_t>& uint32_dist,
                       std::uniform_int_distribution<uint64_t>& uint64_dist,
                       std::uniform_real_distribution<float>& float_dist,
                       std::uniform_real_distribution<double>& double_dist, int depth);

void fillMessageWithRandomValues(google::protobuf::Message* message, int depth = 0);

bool loadSchema(const std::vector<std::byte>& schema_bytes,
                google::protobuf::SimpleDescriptorDatabase* proto_db);

std::vector<uint8_t>
generateRandomProtobufMessageFromSchema(const foxglove::ServiceRequestDefinition& service_definition);

template <typename T>
void setRandomValue(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                    std::mt19937& gen, std::uniform_int_distribution<int32_t>& int32_dist,
                    std::uniform_int_distribution<int64_t>& int64_dist,
                    std::uniform_int_distribution<uint32_t>& uint32_dist,
                    std::uniform_int_distribution<uint64_t>& uint64_dist,
                    std::uniform_real_distribution<float>& float_dist,
                    std::uniform_real_distribution<double>& double_dist) {
  auto reflection = message->GetReflection();
  if constexpr (std::is_same_v<T, bool>) {
    reflection->SetBool(message, field, int32_dist(gen) % 2);
  } else if constexpr (std::is_same_v<T, int32_t>) {
    reflection->SetInt32(message, field, int32_dist(gen));
  } else if constexpr (std::is_same_v<T, int64_t>) {
    reflection->SetInt64(message, field, int64_dist(gen));
  } else if constexpr (std::is_same_v<T, uint32_t>) {
    reflection->SetUInt32(message, field, uint32_dist(gen));
  } else if constexpr (std::is_same_v<T, uint64_t>) {
    reflection->SetUInt64(message, field, uint64_dist(gen));
  } else if constexpr (std::is_same_v<T, float>) {
    reflection->SetFloat(message, field, float_dist(gen));
  } else if constexpr (std::is_same_v<T, double>) {
    reflection->SetDouble(message, field, double_dist(gen));
  } else if constexpr (std::is_same_v<T, std::string>) {
    reflection->SetString(message, field, "random_string");
  }
}

}  // namespace heph::ws_bridge