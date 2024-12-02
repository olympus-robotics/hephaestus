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

enum class PublisherStatus : uint8_t { ALIVE = 0, DROPPED };
struct PublisherInfo {
  std::string topic;
  PublisherStatus status;
};

[[nodiscard]] auto getListOfPublishers(const Session& session, std::string_view topic = "**")
    -> std::vector<PublisherInfo>;

void printPublisherInfo(const PublisherInfo& info);

/// Class to detect all the publisher present in the network.
/// The publisher need to advertise their presence with the liveliness token.
class PublisherDiscovery {
public:
  using Callback = std::function<void(const PublisherInfo& info)>;
  explicit PublisherDiscovery(SessionPtr session, TopicConfig topic_config, Callback&& callback);
  ~PublisherDiscovery();

  PublisherDiscovery(const PublisherDiscovery&) = delete;
  PublisherDiscovery(PublisherDiscovery&&) = delete;
  auto operator=(const PublisherDiscovery&) -> PublisherDiscovery& = delete;
  auto operator=(PublisherDiscovery&&) -> PublisherDiscovery& = delete;

private:
  void createLivelinessSubscriber();

private:
  SessionPtr session_;
  TopicConfig topic_config_;
  Callback callback_;

  std::unique_ptr<::zenoh::Subscriber<void>> liveliness_subscriber_;

  static constexpr std::size_t DEFAULT_CACHE_RESERVES = 100;
  std::unique_ptr<concurrency::MessageQueueConsumer<PublisherInfo>> infos_consumer_;
};

}  // namespace heph::ipc::zenoh
