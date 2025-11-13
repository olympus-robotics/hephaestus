//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/service.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <absl/strings/str_split.h>
#include <fmt/format.h>
#include <zenoh.h>
#include <zenoh/api/base.hxx>
#include <zenoh/api/channels.hxx>
#include <zenoh/api/encoding.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/api/reply.hxx>
#include <zenoh/api/session.hxx>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log/log.h"

namespace heph::ipc::zenoh {
namespace {

constexpr auto TOPIC_INFO_SERVICE_TOPIC_PREFIX = "topic_info";

[[nodiscard]] auto
getServiceCallResponses(const ::zenoh::channels::FifoChannel::HandlerType<::zenoh::Reply>& service_replies)
    -> std::vector<ServiceResponse<std::vector<std::byte>>> {
  std::vector<ServiceResponse<std::vector<std::byte>>> reply_messages;
  for (auto res = service_replies.recv(); std::holds_alternative<::zenoh::Reply>(res);
       res = service_replies.recv()) {
    const auto& reply = std::get<::zenoh::Reply>(res);
    if (!reply.is_ok()) {
      continue;
    }

    const auto& sample = reply.get_ok();
    auto message = toByteVector(sample.get_payload());

    const auto server_topic = static_cast<std::string>(sample.get_keyexpr().as_string_view());
    reply_messages.emplace_back(server_topic, std::move(message));
  }

  return reply_messages;
}
}  // namespace

auto callServiceRaw(Session& session, const TopicConfig& topic_config, std::span<const std::byte> buffer,
                    std::chrono::milliseconds timeout)
    -> std::vector<ServiceResponse<std::vector<std::byte>>> {
  heph::log(heph::DEBUG, "calling service raw", "topic", topic_config.name);

  ::zenoh::Session::GetOptions options{};
  options.timeout_ms = static_cast<uint64_t>(timeout.count());
  options.payload = toZenohBytes(buffer);
  options.encoding = ::zenoh::Encoding::Predefined::zenoh_bytes();

  const ::zenoh::KeyExpr keyexpr(topic_config.name);
  ::zenoh::ZResult result{};
  static constexpr auto FIFO_QUEUE_SIZE = 100;
  auto replies = session.zenoh_session.get(keyexpr, "", ::zenoh::channels::FifoChannel(FIFO_QUEUE_SIZE),
                                           std::move(options), &result);
  if (result != Z_OK) {
    heph::log(heph::ERROR, "failed to call service, server error", "topic", topic_config.name);
    return {};
  }

  return getServiceCallResponses(replies);
}

auto getEndpointTypeInfoServiceTopic(const std::string& topic) -> std::string {
  return fmt::format("{}/{}", TOPIC_INFO_SERVICE_TOPIC_PREFIX, topic);
}

auto isEndpointTypeInfoServiceTopic(const std::string& topic) -> bool {
  std::vector<std::string> elements = absl::StrSplit(topic, '/');
  if (elements.empty()) {
    return false;
  }

  return elements.front() == TOPIC_INFO_SERVICE_TOPIC_PREFIX;
}

auto createTypeInfoService(std::shared_ptr<Session>& session, const TopicConfig& topic_config,
                           Service<std::string, std::string>::Callback&& callback)
    -> std::unique_ptr<Service<std::string, std::string>> {
  if (isEndpointTypeInfoServiceTopic(topic_config.name)) {
    return nullptr;
  }

  auto failure_callback = [&topic_config]() {
    heph::log(heph::ERROR, "Failed to process type info service", "service_topic", topic_config.name);
  };

  auto post_reply_callback = []() {
    // Do nothing.
  };

  const ServiceConfig service_config = {
    .create_liveliness_token = false,
    .create_type_info_service = false,
  };

  return std::make_unique<Service<std::string, std::string>>(
      session, TopicConfig{ getEndpointTypeInfoServiceTopic(topic_config.name) }, std::move(callback),
      failure_callback, post_reply_callback, service_config);
}

}  // namespace heph::ipc::zenoh
