#include "hephaestus/websocket_bridge/ws_server_utils.h"

namespace heph::ws_bridge {

bool convertIpcRawServiceResponseToWsServiceResponse(
    WsServerServiceId service_id, WsServerServiceCallId call_id,
    const ipc::zenoh::ServiceResponse<std::vector<std::byte>>& raw_response,
    WsServerServiceResponse& ws_response) {
  if (raw_response.value.empty()) {
    return false;
  }

  std::vector<uint8_t> response_data(raw_response.value.size());
  std::transform(raw_response.value.begin(), raw_response.value.end(), response_data.begin(),
                 [](std::byte b) { return static_cast<uint8_t>(b); });

  ws_response = {
    .serviceId = service_id,
    .callId = call_id,
    .encoding = std::string("protobuf", 8),
    .data = std::move(response_data),
  };
  return true;
}

}  // namespace heph::ws_bridge