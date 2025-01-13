//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <zenoh/api/queryable.hxx>
#include <zenoh/api/session.hxx>

namespace heph::ipc::zenoh {

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
  bool multicast_scouting_enabled = true;
  std::vector<std::string> connect_endpoints;
  std::vector<std::string> listen_endpoints;
  std::string multicast_scouting_interface = "auto";
};

struct Session {
  ::zenoh::Session zenoh_session;
  Config config;
};

using SessionPtr = std::shared_ptr<Session>;
[[nodiscard]] auto createSession(Config config) -> SessionPtr;

}  // namespace heph::ipc::zenoh
