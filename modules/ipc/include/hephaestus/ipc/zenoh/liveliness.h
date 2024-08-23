//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

enum class PublisherStatus : uint8_t { ALIVE = 0, DROPPED };
struct PublisherInfo {
  std::string topic;
  PublisherStatus status;
};

[[nodiscard]] auto getListOfPublishers(const Session& session,
                                       std::string_view topic = "**") -> std::vector<PublisherInfo>;

void printPublisherInfo(const PublisherInfo& info);

/// Class to detect all the publisher present in the network.
/// The publisher need to advertise their presence with the liveliness token.
class PublisherDiscovery {
public:
  using Callback = std::function<void(const PublisherInfo& info)>;
  /// The callback needs to be thread safe as they may be called in parallel for different publishers
  /// discovered.
  explicit PublisherDiscovery(SessionPtr session, TopicConfig topic_config, Callback&& callback);
  ~PublisherDiscovery() = default;

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
};

}  // namespace heph::ipc::zenoh
