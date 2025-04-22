//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/liveliness.h"

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <absl/strings/str_split.h>
#include <fmt/base.h>
#include <fmt/format.h>
#include <magic_enum.hpp>
#include <zenoh.h>
#include <zenoh/api/channels.hxx>
#include <zenoh/api/closures.hxx>
#include <zenoh/api/enums.hxx>
#include <zenoh/api/id.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/api/reply.hxx>
#include <zenoh/api/sample.hxx>
#include <zenoh/api/subscriber.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"

namespace heph::ipc::zenoh {
namespace {

struct LivelinessTokenKeyexprSuffix {
  static constexpr std::string_view PUBLISHER = "hephaestus_publisher";
  static constexpr std::string_view SUBSCRIBER = "hephaestus_subscriber";
  static constexpr std::string_view SERVICE_SERVER = "hephaestus_service_server";
  static constexpr std::string_view SERVICE_CLIENT = "hephaestus_service_client";
  static constexpr std::string_view ACTION_SERVER = "hephaestus_action_server";
};

[[nodiscard]] auto toEndpointnfoStatus(::zenoh::SampleKind kind) -> EndpointInfo::Status {
  switch (kind) {
    case Z_SAMPLE_KIND_PUT:
      return EndpointInfo::Status::ALIVE;
    case Z_SAMPLE_KIND_DELETE:
      return EndpointInfo::Status::DROPPED;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable() in C++23
}

[[nodiscard]] auto actorTypeToSuffix(EndpointType type) -> std::string_view {
  switch (type) {
    case EndpointType::PUBLISHER:
      return LivelinessTokenKeyexprSuffix::PUBLISHER;
    case EndpointType::SUBSCRIBER:
      return LivelinessTokenKeyexprSuffix::SUBSCRIBER;
    case EndpointType::SERVICE_SERVER:
      return LivelinessTokenKeyexprSuffix::SERVICE_SERVER;
    case EndpointType::SERVICE_CLIENT:
      return LivelinessTokenKeyexprSuffix::SERVICE_CLIENT;
    case EndpointType::ACTION_SERVER:
      return LivelinessTokenKeyexprSuffix::ACTION_SERVER;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable() in C++23
}

auto livelinessTokenKeyexprSuffixTActionType(std::string_view type) -> std::optional<EndpointType> {
  if (type == LivelinessTokenKeyexprSuffix::PUBLISHER) {
    return EndpointType::PUBLISHER;
  }
  if (type == LivelinessTokenKeyexprSuffix::SUBSCRIBER) {
    return EndpointType::SUBSCRIBER;
  }
  if (type == LivelinessTokenKeyexprSuffix::SERVICE_SERVER) {
    return EndpointType::SERVICE_SERVER;
  }
  if (type == LivelinessTokenKeyexprSuffix::ACTION_SERVER) {
    return EndpointType::ACTION_SERVER;
  }
  if (type == LivelinessTokenKeyexprSuffix::SERVICE_CLIENT) {
    return EndpointType::SERVICE_CLIENT;
  }

  return std::nullopt;
}

[[nodiscard]] auto endpointAlreadyDiscovered(const std::vector<EndpointInfo>& endpoints,
                                             const EndpointInfo& info) -> bool {
  return std::ranges::find_if(endpoints, [&info](const EndpointInfo& endpoint) {
           return endpoint == info;
         }) != endpoints.end();
}
}  // namespace

auto generateLivelinessTokenKeyexpr(std::string_view topic, const ::zenoh::Id& session_id,
                                    EndpointType actor_type) -> std::string {
  return fmt::format("{}|{}|{}", topic, toString(session_id), actorTypeToSuffix(actor_type));
}

auto parseLivelinessToken(std::string_view keyexpr, ::zenoh::SampleKind kind) -> std::optional<EndpointInfo> {
  static constexpr auto TOPIC_IDX = 0;
  static constexpr auto SESSION_IDX = 1;
  static constexpr auto TYPE_IDX = 2;
  // Expected keyexpr: <topic/name/whatever>|<session_id>|<actor_type>
  const std::vector<std::string> items = absl::StrSplit(keyexpr, '|');
  if (items.size() != 3) {
    heph::log(heph::ERROR, "invalid liveliness keyexpr, too few items", "keyexpr", keyexpr, "items_count",
              items.size());
    return std::nullopt;
  }

  auto type = livelinessTokenKeyexprSuffixTActionType(items[TYPE_IDX]);
  if (!type) {
    heph::log(heph::ERROR, "invalid liveliness keyexpr, failed to parse type", "keyexpr", keyexpr, "type",
              items[TYPE_IDX]);
    return std::nullopt;
  }

  std::string topic = items[TOPIC_IDX];

  return EndpointInfo{ .session_id = items[SESSION_IDX],
                       .topic = std::move(topic),
                       .type = *type,
                       .status = toEndpointnfoStatus(kind) };
}

auto getListOfEndpoints(const Session& session, const TopicFilter& topic_filter)
    -> std::vector<EndpointInfo> {
  static constexpr auto FIFO_BOUND = 100;
  const ::zenoh::KeyExpr keyexpr("**");
  auto replies = session.zenoh_session.liveliness_get(keyexpr, ::zenoh::channels::FifoChannel(FIFO_BOUND));

  std::vector<EndpointInfo> endpoints;
  for (auto res = replies.recv(); std::holds_alternative<::zenoh::Reply>(res); res = replies.recv()) {
    const auto& reply = std::get<::zenoh::Reply>(res);
    if (!reply.is_ok()) {
      heph::log(heph::ERROR, "invalid reply for liveliness");
      continue;
    }

    const auto& sample = reply.get_ok();
    auto actor_info = parseLivelinessToken(sample.get_keyexpr().as_string_view(), sample.get_kind());
    if (actor_info && topic_filter.isAcceptable(actor_info->topic) &&
        !endpointAlreadyDiscovered(endpoints, *actor_info)) {
      endpoints.push_back(std::move(*actor_info));
    }
  }

  return endpoints;
}

void printEndpointInfo(const EndpointInfo& info) {
  fmt::println("[{}]\t{:<15}\t{}\t'{}'", info.status == EndpointInfo::Status::ALIVE ? "NEW" : "LOST",
               magic_enum::enum_name(info.type), info.session_id, info.topic);
}

EndpointDiscovery::EndpointDiscovery(SessionPtr session, TopicFilter topic_filter, Callback&& callback)
  : session_(std::move(session))
  , topic_filter_(std::move(topic_filter))
  , callback_(std::move(callback))
  , infos_consumer_(std::make_unique<concurrency::MessageQueueConsumer<EndpointInfo>>(
        [this](const EndpointInfo& info) { callback_(info); }, DEFAULT_CACHE_RESERVES)) {
  infos_consumer_->start();
  // NOTE: the liveliness token subscriber is called only when the status of the publisher changes.
  // This means that we won't get the list of publisher that are already running.
  // To do that we need to query the list of publisher beforehand.
  auto publishers_info = getListOfEndpoints(*session_, topic_filter_);
  for (const auto& info : publishers_info) {
    infos_consumer_->queue().forcePush(info);
  }

  // Here we create the subscriber for the liveliness tokens.
  // NOTE: If a publisher start publishing between the previous call and the time needed to start the
  // subscriber, we will loose that publisher.
  // This could be avoided by querying for the list of publisher after we start the subscriber and keeping a
  // track of what we already advertised so not to call the user callback twice on the same event.
  // TODO: implement the optimization described above.
  createLivelinessSubscriber();
}

// NOLINTNEXTLINE(bugprone-exception-escape)
EndpointDiscovery::~EndpointDiscovery() {
  auto stopped = infos_consumer_->stop();
  stopped.get();
  try {
    std::move(*liveliness_subscriber_).undeclare();
  } catch (std::exception& e) {
    heph::log(heph::ERROR, "failed to undeclare liveliness subscriber", "exception", e.what());
  }
}

void EndpointDiscovery::createLivelinessSubscriber() {
  const ::zenoh::KeyExpr keyexpr("**");

  auto liveliness_callback = [this](const ::zenoh::Sample& sample) mutable {
    auto info = parseLivelinessToken(sample.get_keyexpr().as_string_view(), sample.get_kind());
    if (info.has_value() && topic_filter_.isAcceptable(info->topic)) {
      infos_consumer_->queue().forcePush(std::move(*info));
    }
  };

  liveliness_subscriber_ =
      std::make_unique<::zenoh::Subscriber<void>>(session_->zenoh_session.liveliness_declare_subscriber(
          keyexpr, std::move(liveliness_callback), ::zenoh::closures::none));
}

}  // namespace heph::ipc::zenoh
