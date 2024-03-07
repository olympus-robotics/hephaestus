//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <string>

namespace eolo::ipc {

enum class Mode : uint8_t { PEER = 0, CLIENT, ROUTER };
enum class Protocol : uint8_t { ANY = 0, UDP, TCP };

struct Config {
  bool enable_shared_memory = false;  //! NOTE: With shared-memory enabled, the publisher still uses the
                                      //! network transport layer to notify subscribers of the shared-memory
                                      //! segment to read. Therefore, for very small messages, shared -
                                      //! memory transport could be less efficient than using the default
                                      //! network transport to directly carry the payload.
  Mode mode = Mode::PEER;
  // NOLINTNEXTLINE(readability-redundant-string-init) otherwise we need to specify in constructor
  std::string router = "";  //! If specified connect to the given router endpoint.
  std::size_t cache_size = 0;
  bool qos = false;
  bool real_time = false;
  Protocol protocol{ Protocol::ANY };
};

struct TopicConfig {
  std::string name;
};

struct MessageMetadata {
  // TODO: convert this to a uuid
  std::string sender_id;
  std::string topic;
  std::chrono::nanoseconds timestamp{};
  std::size_t sequence_id{};
};

[[nodiscard]] constexpr auto getTypeInfoServiceTopic(const std::string& topic) -> std::string {
  return std::format("type_info/{}", topic);
}

}  // namespace eolo::ipc
