//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <zenoh/api/queryable.hxx>
#include <zenoh/api/session.hxx>

namespace heph::ipc::zenoh {

enum class Mode : uint8_t { PEER = 0, CLIENT, ROUTER };
enum class Protocol : uint8_t { ANY = 0, UDP, TCP };

/// There are a lot of options to configure a zenoh session,
/// See  for more information https://zenoh.io/docs/manual/configuration/#configuration-files
struct ZenohConfig {
  ZenohConfig() {
    zconfig.insert_json5(Z_CONFIG_ADD_TIMESTAMP_KEY, "true");
  }
  explicit ZenohConfig(std::string const& path) : zconfig(::zenoh::Config::from_file(path)) {
  }

  ~ZenohConfig() noexcept = default;
  ZenohConfig(const ZenohConfig&) = delete;
  auto operator=(const ZenohConfig&) -> ZenohConfig& = delete;
  ZenohConfig(ZenohConfig&&) noexcept = default;
  auto operator=(ZenohConfig&&) noexcept -> ZenohConfig& = default;

  ::zenoh::Config zconfig{ ::zenoh::Config::create_default() };
  std::size_t cache_size{ 0 };
};

void setSessionId(ZenohConfig& config, std::string_view id);
void setSessionIdFromBinary(ZenohConfig& config);
void setSharedMemory(ZenohConfig& config, bool);
void setMode(ZenohConfig& config, Mode mode);
void connectToEndpoints(ZenohConfig& config, const std::vector<std::string>& endpoints);
void listenToEndpoints(ZenohConfig& config, const std::vector<std::string>& endpoints);
void setQos(ZenohConfig& config, bool);
void setRealTime(ZenohConfig& config, bool);
void setMulticastScouting(ZenohConfig& config, bool);
void setMulticastScoutingInterface(ZenohConfig& config, std::string_view interface);

struct Config {
  bool use_binary_name_as_session_id = false;
  std::optional<std::string> id{ std::nullopt };
  std::string zenoh_config_path;
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
  std::string multicast_scouting_interface = "auto";
};

struct Session {
  ::zenoh::Session zenoh_session;
  std::size_t cache_size;
};

/// Create configuration for a session that doesn't connect to any other session.
/// This is useful for testing and for local communications
[[nodiscard]] auto createLocalConfig() -> Config;

using SessionPtr = std::shared_ptr<Session>;
[[nodiscard]] auto createSession(Config config) -> SessionPtr;
[[nodiscard]] auto createSession(ZenohConfig config) -> SessionPtr;

}  // namespace heph::ipc::zenoh
