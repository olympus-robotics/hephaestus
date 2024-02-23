//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include "eolo/ipc/zenoh/session.h"

namespace eolo::ipc::zenoh {

enum class PublisherStatus : uint8_t { ALIVE = 0, DROPPED };
struct PublisherInfo {
  std::string topic;
  PublisherStatus status;
};

[[nodiscard]] auto getListOfPublishers(const Session& session) -> std::vector<PublisherInfo>;

void printPublisherInfo(const PublisherInfo& info);

/// Class to detect all the publisher present in the network.
/// The publisher need to advertise their presence with the liveliness token.
class PublisherDiscovery {
public:
  using Callback = std::function<void(const PublisherInfo& info)>;
  /// The callback needs to be thread safe as they maybe called in parallel for different publishers
  /// discovered.
  explicit PublisherDiscovery(SessionPtr session, Callback&& callback);

private:
  void createLivelinessSubscriber();

private:
  SessionPtr session_;
  Callback callback_;

  z_owned_subscriber_t liveliness_subscriber_{};
};

}  // namespace eolo::ipc::zenoh
