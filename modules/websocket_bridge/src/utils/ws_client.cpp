//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/ws_client.h"

namespace heph::ws_bridge {

ServiceCallState::ServiceCallState(uint32_t call_id)
  : call_id(call_id), status(Status::DISPATCHED), dispatch_time(std::chrono::steady_clock::now()) {
}

std::optional<std::unique_ptr<google::protobuf::Message>> ServiceCallState::receiveResponse(
    const WsServerServiceResponse _response, WsServerAdvertisements& ws_server_ads) {
  if (_response.callId != call_id) {
    heph::log(heph::ERROR, "Mismatched call ID", "expected_call_id", call_id, "received_call_id",
              _response.callId);
    return std::nullopt;
  }

  if (_response.encoding != "protobuf") {
    heph::log(heph::ERROR, "Unexpected encoding in service response", "expected", "protobuf", "received",
              _response.encoding);
    status = Status::FAILED;
    return std::nullopt;
  }

  auto message = retrieveResponseMessageFromDatabase(_response.serviceId, ws_server_ads.schema_db);
  if (!message) {
    heph::log(heph::ERROR, "Failed to response retrieve message from database", "call_id", call_id,
              "service_id", _response.serviceId);
    status = Status::FAILED;
    return std::nullopt;
  }

  if (!message->ParseFromArray(_response.data.data(), static_cast<int>(_response.data.size()))) {
    heph::log(heph::ERROR, "Failed to parse response data with proto schema", "call_id", call_id, "data_size",
              _response.data.size(), "schema_name", message->GetDescriptor()->full_name());

    status = Status::FAILED;
    return std::nullopt;
  }

  response = _response;
  response_time = std::chrono::steady_clock::now();
  status = Status::SUCCESS;

  return message;
}

void ServiceCallState::receiveFailureResponse(const std::string& error_msg) {
  response_time = std::chrono::steady_clock::now();
  error_message = error_msg;
  status = Status::FAILED;
}

bool ServiceCallState::hasResponse() const {
  const bool has_response = (status == Status::SUCCESS || status == Status::FAILED);

  if (has_response && (!response.has_value() && error_message.empty())) {
    heph::log(heph::ERROR, "Service call has terminated, but neither has a response nor an error msg.",
              "call_id", call_id);
    return false;
  }

  return has_response;
}

bool ServiceCallState::wasSuccessful() const {
  return status == Status::SUCCESS;
}

bool ServiceCallState::hasFailed() const {
  return status == Status::FAILED;
}

std::optional<std::chrono::milliseconds> ServiceCallState::getDurationMs() const {
  if (!hasResponse()) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                 dispatch_time);
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(response_time - dispatch_time);
}

std::string horizontalLine(uint32_t cell_content_width, uint32_t columns) {
  std::stringstream ss;
  ss << "+";
  for (uint32_t col = 0; col < columns; ++col) {
    for (uint32_t i = 0; i < cell_content_width; ++i) {
      ss << "-";
    }
    ss << "+";
  }
  ss << "\n";
  return ss.str();
}

void printServiceCallStateMap(ServiceCallStateMap& state) {
  constexpr uint32_t max_columns = 5;
  constexpr uint32_t cell_content_width = 17;

  const uint32_t num_service_calls = static_cast<uint32_t>(state.size());
  const uint32_t width = std::min(num_service_calls, max_columns);
  const uint32_t height = (num_service_calls + width - 1) / width;

  fmt::println("Service Call States");

  auto horizontal_line = horizontalLine(cell_content_width, width);
  fmt::print("{}", horizontal_line);

  auto it = state.begin();

  for (uint32_t row_idx = 0; row_idx < height && it != state.end(); ++row_idx) {
    fmt::print("|");
    for (uint32_t col_idx = 0; col_idx < width; ++col_idx) {
      // If we already reached the end of the map, print empty cell.
      if (it == state.end()) {
        fmt::print("{:<{}}|", " ", cell_content_width);
        continue;
      }

      // Print the valid cell content.
      const auto& service_call_state = it->second;
      const uint32_t call_id = service_call_state.call_id;

      const std::string status_str =
          service_call_state.hasResponse() ? (service_call_state.wasSuccessful() ? "✔" : "✖") : "∅";

      fmt::print("{:<{}}|",
                 fmt::format(" {:03}  {:1}  {:4}ms ", call_id, status_str,
                             service_call_state.getDurationMs().value().count()),
                 cell_content_width);

      ++it;
    }
    fmt::print("\n{}", horizontal_line);
  }
}

void printAdvertisedServices(const WsServerAdvertisements& ws_server_ads) {
  fmt::println("Advertised services:");
  fmt::println("--------------------------------------------------");
  if (ws_server_ads.services.empty()) {
    fmt::println("No services advertised.");
    fmt::println("--------------------------------------------------");
    return;
  }
  for (const auto& [service_id, service] : ws_server_ads.services) {
    fmt::println("Service ID   : {}", service_id);
    fmt::println("Name         : {}", service.name);
    fmt::println("Type         : {}", service.type);
    if (service.request.has_value()) {
      fmt::println("Request:");
      fmt::println("  Encoding      : {}", service.request->encoding);
      fmt::println("  Schema Name   : {}", service.request->schemaName);
      fmt::println("  Schema Enc.   : {}", service.request->schemaEncoding);
    } else {
      fmt::println("Request      : None");
    }
    if (service.response.has_value()) {
      fmt::println("Response:");
      fmt::println("  Encoding      : {}", service.response->encoding);
      fmt::println("  Schema Name   : {}", service.response->schemaName);
      fmt::println("  Schema Enc.   : {}", service.response->schemaEncoding);
    } else {
      fmt::println("Response     : None");
    }
    fmt::println("--------------------------------------------------");
  }
}

void printAdvertisedTopics(const WsServerAdvertisements& ws_server_ads) {
  fmt::println("Advertised topics:");
  fmt::println("--------------------------------------------------");
  if (ws_server_ads.channels.empty()) {
    fmt::println("No topics advertised.");
    fmt::println("--------------------------------------------------");
    return;
  }
  for (const auto& [channelId, channel] : ws_server_ads.channels) {
    fmt::println("Channel ID : {}", channelId);
    fmt::println("Topic      : {}", channel.topic);
    fmt::println("Encoding   : {}", channel.encoding);
    fmt::println("Schema Name: {}", channel.schemaName);
    fmt::println("--------------------------------------------------");
  }
}

bool allServiceCallsFinished(const ServiceCallStateMap& state) {
  for (const auto& [call_id, service_call_state] : state) {
    if (!service_call_state.hasResponse()) {
      return false;
    }
  }
  return true;
}

}  // namespace heph::ws_bridge