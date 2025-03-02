//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/ws_client.h"

namespace heph::ws {

ServiceCallState::ServiceCallState(uint32_t call_id)
  : call_id(call_id), status(Status::DISPATCHED), dispatch_time(std::chrono::steady_clock::now()) {
}

std::optional<std::unique_ptr<google::protobuf::Message>> ServiceCallState::receiveResponse(
    const WsServerServiceResponse& service_response, WsServerAdvertisements& ws_server_ads) {
  if (service_response.callId != call_id) {
    heph::log(heph::ERROR, "Mismatched call ID", "expected_call_id", call_id, "received_call_id",
              service_response.callId);
    return std::nullopt;
  }

  if (service_response.encoding != "protobuf") {
    heph::log(heph::ERROR, "Unexpected encoding in service response", "expected", "protobuf", "received",
              service_response.encoding);
    status = Status::FAILED;
    return std::nullopt;
  }

  auto message = retrieveResponseMessageFromDatabase(service_response.serviceId, ws_server_ads.schema_db);
  if (!message) {
    heph::log(heph::ERROR, "Failed to response retrieve message from database", "call_id", call_id,
              "service_id", service_response.serviceId);
    status = Status::FAILED;
    return std::nullopt;
  }

  if (!message->ParseFromArray(service_response.data.data(),
                               static_cast<int>(service_response.data.size()))) {
    heph::log(heph::ERROR, "Failed to parse response data with proto schema", "call_id", call_id, "data_size",
              service_response.data.size(), "schema_name", message->GetDescriptor()->full_name());

    status = Status::FAILED;
    return std::nullopt;
  }

  response = service_response;
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

auto ServiceCallState::wasSuccessful() const -> bool {
  return status == Status::SUCCESS;
}

auto ServiceCallState::hasFailed() const -> bool {
  return status == Status::FAILED;
}

auto ServiceCallState::getDurationMs() const -> std::optional<std::chrono::milliseconds> {
  if (!hasResponse()) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                 dispatch_time);
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(response_time - dispatch_time);
}

auto horizontalLine(uint32_t cell_content_width, uint32_t columns) -> std::string {
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
  constexpr uint32_t MAX_COLUMNS = 5;
  constexpr uint32_t CELL_CONTENT_WIDTH = 17;

  const auto num_service_calls = static_cast<uint32_t>(state.size());
  const uint32_t width = std::min(num_service_calls, MAX_COLUMNS);
  const uint32_t height = (num_service_calls + width - 1) / width;

  fmt::println("Service Call States");

  auto horizontal_line = horizontalLine(CELL_CONTENT_WIDTH, width);
  fmt::print("{}", horizontal_line);

  auto it = state.begin();

  for (uint32_t row_idx = 0; row_idx < height && it != state.end(); ++row_idx) {
    fmt::print("|");
    for (uint32_t col_idx = 0; col_idx < width; ++col_idx) {
      // If we already reached the end of the map, print empty cell.
      if (it == state.end()) {
        fmt::print("{:<{}}|", " ", CELL_CONTENT_WIDTH);
        continue;
      }

      // Print the valid cell content.
      const auto& service_call_state = it->second;
      const uint32_t call_id = service_call_state.call_id;

      const std::string success_or_fail_str = service_call_state.wasSuccessful() ? "✔" : "✖";
      const std::string status_str = service_call_state.hasResponse() ? success_or_fail_str : "∅";

      fmt::print("{:<{}}|",
                 fmt::format(" {:03}  {:1}  {:4}ms ", call_id, status_str,
                             service_call_state.getDurationMs().value().count()),
                 CELL_CONTENT_WIDTH);

      ++it;
    }
    fmt::print("\n{}", horizontal_line);
  }
}

void printAdvertisedServices(const WsServerAdvertisements& ws_server_ads) {
  static constexpr size_t SCHEMA_TRUNCATION_DIGITS = 10;

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
      if (service.request->schema.empty()) {
        fmt::println("Schema     : None");
      } else {
        fmt::println(
            "Schema     : {}...{}", service.request->schema.substr(0, SCHEMA_TRUNCATION_DIGITS),
            service.request->schema.substr(service.request->schema.size() - SCHEMA_TRUNCATION_DIGITS));
      }
    } else {
      fmt::println("Request      : None");
    }
    if (service.response.has_value()) {
      fmt::println("Response:");
      fmt::println("  Encoding      : {}", service.response->encoding);
      fmt::println("  Schema Name   : {}", service.response->schemaName);
      fmt::println("  Schema Enc.   : {}", service.response->schemaEncoding);
      if (service.response->schema.empty()) {
        fmt::println("Schema     : None");
      } else {
        fmt::println(
            "Schema     : {}...{}", service.response->schema.substr(0, SCHEMA_TRUNCATION_DIGITS),
            service.response->schema.substr(service.response->schema.size() - SCHEMA_TRUNCATION_DIGITS));
      }
    } else {
      fmt::println("Response     : None");
    }
    fmt::println("--------------------------------------------------");
  }
}

void printAdvertisedTopics(const WsServerAdvertisements& ws_server_ads) {
  static constexpr size_t SCHEMA_TRUNCATION_DIGITS = 10;

  fmt::println("Advertised topics:");
  fmt::println("--------------------------------------------------");
  if (ws_server_ads.channels.empty()) {
    fmt::println("No topics advertised.");
    fmt::println("--------------------------------------------------");
    return;
  }
  for (const auto& [channelId, channel] : ws_server_ads.channels) {
    fmt::println("Channel ID     : {}", channelId);
    fmt::println("Topic          : {}", channel.topic);
    fmt::println("Encoding       : {}", channel.encoding);
    fmt::println("Schema Name    : {}", channel.schemaName);
    if (channel.schemaEncoding.has_value()) {
      fmt::println("Schema Enc.    : {}", channel.schemaEncoding.value());
    } else {
      fmt::println("Schema Enc.    : None");
    }
    if (channel.schema.empty()) {
      fmt::println("Schema         : None");
    } else {
      fmt::println("Schema         : {}...{}", channel.schema.substr(0, SCHEMA_TRUNCATION_DIGITS),
                   channel.schema.substr(channel.schema.size() - SCHEMA_TRUNCATION_DIGITS));
    }
    fmt::println("--------------------------------------------------");
  }
}

void printClientChannelAds(const std::vector<WsServerClientChannelAd>& client_ads) {
  static constexpr size_t SCHEMA_TRUNCATION_DIGITS = 10;

  fmt::println("Client Channel Advertisements:");
  fmt::println("--------------------------------------------------");
  if (client_ads.empty()) {
    fmt::println("No client channels advertised.");
    fmt::println("--------------------------------------------------");
    return;
  }
  for (const auto& ad : client_ads) {
    fmt::println("Client Channel ID : {}", ad.channelId);
    fmt::println("Topic             : {}", ad.topic);
    fmt::println("Encoding          : {}", ad.encoding);
    fmt::println("Schema Name       : {}", ad.schemaName);
    if (ad.schemaEncoding.has_value()) {
      fmt::println("Schema Enc.       : {}", ad.schemaEncoding.value());
    } else {
      fmt::println("Schema Enc.       : None");
    }
    if (ad.schema.has_value()) {
      fmt::println("Schema            : {}...{}", ad.schema->substr(0, SCHEMA_TRUNCATION_DIGITS),
                   ad.schema->substr(ad.schema->size() - SCHEMA_TRUNCATION_DIGITS));
    } else {
      fmt::println("Schema            : None");
    }
    fmt::println("--------------------------------------------------");
  }
}

auto allServiceCallsFinished(const ServiceCallStateMap& state) -> bool {
  return std::ranges::all_of(state, [](const auto& pair) { return pair.second.hasResponse(); });
}

}  // namespace heph::ws