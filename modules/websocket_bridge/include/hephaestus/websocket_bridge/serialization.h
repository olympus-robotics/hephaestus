#pragma once

#include <cstddef>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#include <absl/log/check.h>
#include <absl/log/log.h>
#include <absl/strings/ascii.h>
#include <fmt/base.h>
#include <foxglove/websocket/base64.hpp>
#include <foxglove/websocket/serialization.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <hephaestus/serdes/type_info.h>
#include <magic_enum.hpp>

namespace heph::ws_bridge {

std::string convertProtoBytesToFoxgloveBase64String(const std::vector<std::byte>& data);

std::string convertSerializationTypeToString(const serdes::TypeInfo::Serialization& serialization);

void debugPrintSchema(const std::vector<std::byte>& schema);

void printBinary(const uint8_t* data, size_t length);

std::string getTimestampString();

}  // namespace heph::ws_bridge
