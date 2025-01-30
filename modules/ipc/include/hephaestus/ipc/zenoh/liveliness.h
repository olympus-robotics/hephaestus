//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "zenoh/api/subscriber.hxx"

namespace heph::ipc::zenoh {

enum class ActorType : uint8_t { PUBLISHER = 0, SUBSCRIBER, SERVICE, ACTION_SERVER };

struct ActorInfo {
  enum class Status : uint8_t { ALIVE = 0, DROPPED };
  std::string session_id;
  std::string topic;
  ActorType type;
  Status status;
};

[[nodiscard]] auto generateLivelinessTokenKeyexpr(std::string_view topic, const ::zenoh::Id& session_id,
                                                  ActorType actor_type) -> std::string;

[[nodiscard]] auto parseLivelinessTokenKeyexpr(std::string_view keyexpr) -> std::optional<ActorInfo>;

[[nodiscard]] auto getListOfActors(const Session& session, std::string_view topic = "**")
    -> std::vector<ActorInfo>;

void printActorInfo(const ActorInfo& info);

/// Class to detect all the publisher present in the network.
/// The publisher need to advertise their presence with the liveliness token.
class ActorDiscovery {
public:
  using Callback = std::function<void(const ActorInfo& info)>;
  explicit ActorDiscovery(SessionPtr session, TopicConfig topic_config, Callback&& callback);
  ~ActorDiscovery();

  ActorDiscovery(const ActorDiscovery&) = delete;
  ActorDiscovery(ActorDiscovery&&) = delete;
  auto operator=(const ActorDiscovery&) -> ActorDiscovery& = delete;
  auto operator=(ActorDiscovery&&) -> ActorDiscovery& = delete;

private:
  void createLivelinessSubscriber();

private:
  SessionPtr session_;
  TopicConfig topic_config_;
  Callback callback_;

  std::unique_ptr<::zenoh::Subscriber<void>> liveliness_subscriber_;

  static constexpr std::size_t DEFAULT_CACHE_RESERVES = 100;
  std::unique_ptr<concurrency::MessageQueueConsumer<ActorInfo>> infos_consumer_;
};

}  // namespace heph::ipc::zenoh
