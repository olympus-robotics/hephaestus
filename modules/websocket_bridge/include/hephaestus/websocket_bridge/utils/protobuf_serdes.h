//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <absl/log/log.h>
#include <fmt/core.h>
#include <foxglove/websocket/common.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>

#include "hephaestus/serdes/type_info.h"
#include "hephaestus/telemetry/log.h"

namespace heph::ws {

struct RandomGenerators {
  std::mt19937 gen;
  std::uniform_int_distribution<int32_t> int32_dist;
  std::uniform_int_distribution<int64_t> int64_dist;
  std::uniform_int_distribution<uint32_t> uint32_dist;
  std::uniform_int_distribution<uint64_t> uint64_dist;
  std::uniform_real_distribution<float> float_dist;
  std::uniform_real_distribution<double> double_dist;

  RandomGenerators(int min, int max);
};

struct ProtobufSchemaDatabase {
  std::unordered_map<foxglove::ServiceId, std::pair<std::string, std::string>> service_id_to_schema_names;
  std::unordered_map<foxglove::ChannelId, std::string> channel_id_to_schema_name;

  std::unique_ptr<google::protobuf::SimpleDescriptorDatabase> proto_db;
  std::unique_ptr<google::protobuf::DescriptorPool> proto_pool;
  std::unique_ptr<google::protobuf::DynamicMessageFactory> proto_factory;

  ProtobufSchemaDatabase();
  ~ProtobufSchemaDatabase() = default;
};

auto saveSchemaToDatabase(const foxglove::Channel& channel_definition, ProtobufSchemaDatabase& schema_db)
    -> bool;

auto saveSchemaToDatabase(const foxglove::Service& service_definition, ProtobufSchemaDatabase& schema_db)
    -> bool;

auto saveSchemaToDatabase(const foxglove::ServiceResponseDefinition& service_request_definition,
                          ProtobufSchemaDatabase& schema_db) -> bool;

[[nodiscard]] auto retrieveRequestMessageFromDatabase(foxglove::ServiceId service_id,
                                                      const ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message>;

[[nodiscard]] auto retrieveResponseMessageFromDatabase(foxglove::ServiceId service_id,
                                                       const ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message>;

[[nodiscard]] auto retrieveMessageFromDatabase(const std::string& schema_name,
                                               const ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message>;

[[nodiscard]] auto retrieveSchemaNamesFromServiceId(foxglove::ServiceId service_id,
                                                    const ProtobufSchemaDatabase& schema_db)
    -> std::pair<std::string, std::string>;
[[nodiscard]] auto retrieveSchemaNameFromChannelId(foxglove::ChannelId channel_id,
                                                   const ProtobufSchemaDatabase& schema_db) -> std::string;

template <typename T>
void setRandomValue(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                    RandomGenerators& generators);

void fillRepeatedField(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                       RandomGenerators& generators, int depth);

void fillMessageWithRandomValues(google::protobuf::Message* message, RandomGenerators& generators,
                                 int depth = 0);

[[nodiscard]] auto generateRandomMessageFromSchemaName(const std::string& schema_name,
                                                       ProtobufSchemaDatabase& schema_db)
    -> std::unique_ptr<google::protobuf::Message>;

template <typename T>
void setRandomValue(google::protobuf::Message* message, const google::protobuf::FieldDescriptor* field,
                    RandomGenerators& generators) {
  const auto* reflection = message->GetReflection();
  if constexpr (std::is_same_v<T, bool>) {
    reflection->SetBool(message, field, generators.int32_dist(generators.gen) % 2);
  } else if constexpr (std::is_same_v<T, int32_t>) {
    reflection->SetInt32(message, field, generators.int32_dist(generators.gen));
  } else if constexpr (std::is_same_v<T, int64_t>) {
    reflection->SetInt64(message, field, generators.int64_dist(generators.gen));
  } else if constexpr (std::is_same_v<T, uint32_t>) {
    reflection->SetUInt32(message, field, generators.uint32_dist(generators.gen));
  } else if constexpr (std::is_same_v<T, uint64_t>) {
    reflection->SetUInt64(message, field, generators.uint64_dist(generators.gen));
  } else if constexpr (std::is_same_v<T, float>) {
    reflection->SetFloat(message, field, generators.float_dist(generators.gen));
  } else if constexpr (std::is_same_v<T, double>) {
    reflection->SetDouble(message, field, generators.double_dist(generators.gen));
  } else if constexpr (std::is_same_v<T, std::string>) {
    reflection->SetString(message, field, "random_string");
  }
}

[[nodiscard]] auto convertProtoBytesToFoxgloveBase64String(const std::vector<std::byte>& data) -> std::string;

[[nodiscard]] auto convertSerializationTypeToString(const serdes::TypeInfo::Serialization& serialization)
    -> std::string;

void debugPrintSchema(const std::vector<std::byte>& schema);

void debugPrintMessage(const google::protobuf::Message& message);

void printBinary(const uint8_t* data, size_t length);

[[nodiscard]] auto getTimestampString() -> std::string;

[[nodiscard]] auto convertIpcTypeInfoToWsChannelInfo(const std::string& topic,
                                                     const serdes::TypeInfo& type_info)
    -> foxglove::ChannelWithoutId;

[[nodiscard]] auto convertWsChannelInfoToIpcTypeInfo(const foxglove::ClientAdvertisement& channel_info)
    -> std::optional<serdes::TypeInfo>;

}  // namespace heph::ws
