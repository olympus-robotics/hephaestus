//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <zenoh/api/enums.hxx>
#include <zenoh/api/id.hxx>
#include <zenoh/api/subscriber.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

enum class EndpointType : uint8_t {
  PUBLISHER = 0,
  SUBSCRIBER,
  SERVICE_SERVER,
  SERVICE_CLIENT,
  ACTION_SERVER
};

struct EndpointInfo {
  enum class Status : uint8_t { ALIVE = 0, DROPPED };
  std::string session_id;
  std::string topic;
  EndpointType type;
  Status status;

  [[nodiscard]] auto operator==(const EndpointInfo&) const -> bool = default;
};

[[nodiscard]] auto generateLivelinessTokenKeyexpr(std::string_view topic, const ::zenoh::Id& session_id,
                                                  EndpointType actor_type) -> std::string;

[[nodiscard]] auto parseLivelinessToken(std::string_view keyexpr, ::zenoh::SampleKind kind)
    -> std::optional<EndpointInfo>;

[[nodiscard]] auto getListOfEndpoints(const Session& session, std::string_view topic = "**")
    -> std::vector<EndpointInfo>;

void printActorInfo(const EndpointInfo& info);

/// Class to detect all the publisher present in the network.
/// The publisher need to advertise their presence with the liveliness token.
class EndpointDiscovery {
public:
  using Callback = std::function<void(const EndpointInfo& info)>;
  explicit EndpointDiscovery(SessionPtr session, TopicConfig topic_config, Callback&& callback);
  ~EndpointDiscovery();

  EndpointDiscovery(const EndpointDiscovery&) = delete;
  EndpointDiscovery(EndpointDiscovery&&) = delete;
  auto operator=(const EndpointDiscovery&) -> EndpointDiscovery& = delete;
  auto operator=(EndpointDiscovery&&) -> EndpointDiscovery& = delete;

private:
  void createLivelinessSubscriber();

private:
  SessionPtr session_;
  TopicConfig topic_config_;
  Callback callback_;

  std::unique_ptr<::zenoh::Subscriber<void>> liveliness_subscriber_;

  static constexpr std::size_t DEFAULT_CACHE_RESERVES = 100;
  std::unique_ptr<concurrency::MessageQueueConsumer<EndpointInfo>> infos_consumer_;
};

}  // namespace heph::ipc::zenoh
