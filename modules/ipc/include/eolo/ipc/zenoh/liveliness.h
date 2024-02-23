//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/session.h"

namespace eolo::ipc::zenoh {

enum class PublisherStatus : uint8_t { ALIVE = 0, DROPPED };
struct PublisherInfo {
  std::string topic;
  PublisherStatus status;
};

[[nodiscard]] auto getListOfPublishers(const Session& session) -> std::vector<PublisherInfo>;

void printPublisherInfo(const PublisherInfo& info);

class DiscoverPublishers {
public:
  using Callback = std::function<void(const PublisherInfo& info)>;
  explicit DiscoverPublishers(SessionPtr session, Callback&& callback);

private:
  SessionPtr session_;
  Callback callback_;

  z_owned_subscriber_t liveliness_subscriber_{};
};

}  // namespace eolo::ipc::zenoh
