//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/liveliness.h"

#include <algorithm>
#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
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
  static constexpr std::string_view SERVICE = "hephaestus_service";
  static constexpr std::string_view ACTION_SERVER = "hephaestus_actor_server";
};

[[nodiscard]] auto toActorInfoStatus(::zenoh::SampleKind kind) -> ActorInfo::Status {
  switch (kind) {
    case Z_SAMPLE_KIND_PUT:
      return ActorInfo::Status::ALIVE;
    case Z_SAMPLE_KIND_DELETE:
      return ActorInfo::Status::DROPPED;
  }

  __builtin_unreachable();  // TODO(C++23): replace with std::unreachable() in C++23
}

[[nodiscard]] auto actorTypeToSuffix(ActorType type) -> std::string_view {
  switch (type) {
    case ActorType::PUBLISHER:
      return LivelinessTokenKeyexprSuffix::PUBLISHER;
    case ActorType::SUBSCRIBER:
      return LivelinessTokenKeyexprSuffix::SUBSCRIBER;
    case ActorType::SERVICE:
      return LivelinessTokenKeyexprSuffix::SERVICE;
    case ActorType::ACTION_SERVER:
      return LivelinessTokenKeyexprSuffix::ACTION_SERVER;
  }
}

auto livelinessTokenKeyexprSuffixTActionType(std::string_view type) -> std::optional<ActorType> {
  if (type == LivelinessTokenKeyexprSuffix::PUBLISHER) {
    return ActorType::PUBLISHER;
  }
  if (type == LivelinessTokenKeyexprSuffix::SUBSCRIBER) {
    return ActorType::SUBSCRIBER;
  }
  if (type == LivelinessTokenKeyexprSuffix::SERVICE) {
    return ActorType::SERVICE;
  }
  if (type == LivelinessTokenKeyexprSuffix::ACTION_SERVER) {
    return ActorType::ACTION_SERVER;
  }

  return std::nullopt;
}

[[nodiscard]] auto parseLivelinessToken(const ::zenoh::Sample& sample) -> std::optional<ActorInfo> {
  // Expected keyexpr: <topic/name/whatever>/<session_id>/<actor_type>
  const auto keyexpr = sample.get_keyexpr().as_string_view();
  const std::vector<std::string> items = absl::StrSplit(keyexpr, '/');
  if (items.size() < 3) {
    heph::log(heph::ERROR, "invalid liveliness keyexpr", "keyexpr", keyexpr);
    return std::nullopt;
  }

  auto type = livelinessTokenKeyexprSuffixTActionType(items.back());
  if (!type) {
    heph::log(heph::ERROR, "invalid liveliness keyexpr", "keyexpr", keyexpr);
    return std::nullopt;
  }

  std::string topic = items[0];
  for (const auto& str : items | std::views::drop(1) | std::views::take(items.size() - 3)) {
    topic = fmt::format("{}/{}", topic, str);
  }

  return ActorInfo{ .session_id = items[items.size() - 2],
                    .topic = topic,
                    .type = *type,
                    .status = toActorInfoStatus(sample.get_kind()) };
}

}  // namespace

auto generateLivelinessTokenKeyexpr(std::string_view topic, const ::zenoh::Id& session_id,
                                    ActorType actor_type) -> std::string {
  return fmt::format("{}/{}/{}", topic, toString(session_id), actorTypeToSuffix(actor_type));
  // return fmt::format("{}/{}/{}", topic, session_id.to_string(), actorTypeToSuffix(actor_type));
}

auto getListOfActors(const Session& session, std::string_view topic) -> std::vector<ActorInfo> {
  static constexpr auto FIFO_BOUND = 100;
  const ::zenoh::KeyExpr keyexpr(topic);
  auto replies = session.zenoh_session.liveliness_get(keyexpr, ::zenoh::channels::FifoChannel(FIFO_BOUND));

  std::vector<ActorInfo> actors;
  for (auto res = replies.recv(); std::holds_alternative<::zenoh::Reply>(res); res = replies.recv()) {
    const auto& reply = std::get<::zenoh::Reply>(res);
    if (!reply.is_ok()) {
      heph::log(heph::ERROR, "invalid reply for liveliness", "topic", topic);
      continue;
    }

    const auto& sample = reply.get_ok();
    auto actor_info = parseLivelinessToken(sample);
    if (actor_info) {
      actors.push_back(std::move(*actor_info));
    }
  }

  return actors;
}

void printActorInfo(const ActorInfo& info) {
  auto text = fmt::format("[{}] Session: {} Topic: {}", magic_enum::enum_name(info.type), info.session_id,
                          info.topic);
  if (info.status == ActorInfo::Status::DROPPED) {
    text = fmt::format("{} - DROPPED", text);
  }

  fmt::println("{}", text);
}

ActorDiscovery::ActorDiscovery(SessionPtr session, TopicConfig topic_config /* = "**"*/, Callback&& callback)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , infos_consumer_(std::make_unique<concurrency::MessageQueueConsumer<ActorInfo>>(
        [this](const ActorInfo& info) { callback_(info); }, DEFAULT_CACHE_RESERVES)) {
  infos_consumer_->start();
  // NOTE: the liveliness token subscriber is called only when the status of the publisher changes.
  // This means that we won't get the list of publisher that are already running.
  // To do that we need to query the list of publisher beforehand.
  auto publishers_info = getListOfActors(*session_, topic_config_.name);
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
ActorDiscovery::~ActorDiscovery() {
  auto stopped = infos_consumer_->stop();
  stopped.get();
  try {
    std::move(*liveliness_subscriber_).undeclare();
  } catch (std::exception& e) {
    heph::log(heph::ERROR, "failed to undeclare liveliness subscriber", "topic", topic_config_.name,
              "exception", e.what());
  }
}

void ActorDiscovery::createLivelinessSubscriber() {
  const ::zenoh::KeyExpr keyexpr(topic_config_.name);

  auto liveliness_callback = [this](const ::zenoh::Sample& sample) mutable {
    auto info = parseLivelinessToken(sample);
    if (info.has_value()) {
      infos_consumer_->queue().forcePush(std::move(*info));
    }
  };

  liveliness_subscriber_ =
      std::make_unique<::zenoh::Subscriber<void>>(session_->zenoh_session.liveliness_declare_subscriber(
          keyexpr, std::move(liveliness_callback), ::zenoh::closures::none));
}

}  // namespace heph::ipc::zenoh
