#include "hephaestus/websocket_bridge/websocket_server_utils.h"

#include "foxglove/websocket/server_interface.hpp"
#include "hephaestus/websocket_bridge/config.h"

namespace heph::ws_bridge {

foxglove::ServerOptions getWsServerOptions(const WsBridgeConfig& config) {
  // NOTE: Unfortunately 'address' and 'port' are not part of
  // foxglove::ServerOptions and need to be passed to the server when calling
  // "start".

  foxglove::ServerOptions option;

  // Exposed parameters
  option.clientTopicWhitelistPatterns = parseRegexStrings(config.ws_server_client_topic_whitelist);
  option.supportedEncodings = config.ws_server_supported_encodings;
  option.useCompression = config.ws_server_use_compression;

  // Hardcoded parameters
  option.sendBufferLimitBytes = foxglove::DEFAULT_SEND_BUFFER_LIMIT_BYTES;
  option.useTls = false;
  option.certfile = "";
  option.keyfile = "";
  option.sessionId = "session_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
  option.capabilities = {
    // foxglove::CAPABILITY_CLIENT_PUBLISH,
    // foxglove::CAPABILITY_PARAMETERS,
    // foxglove::CAPABILITY_PARAMETERS_SUBSCRIBE,
    // foxglove::CAPABILITY_SERVICES,
    foxglove::CAPABILITY_CONNECTION_GRAPH,
    // foxglove::CAPABILITY_ASSETS
  };
  return option;
}

}  // namespace heph::ws_bridge