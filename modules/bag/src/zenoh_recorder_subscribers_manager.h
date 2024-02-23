//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <future>
#include <span>
#include <unordered_map>

#include <absl/base/thread_annotations.h>

#include "eolo/bag/topic_filter.h"
#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/liveliness.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/subscriber.h"

namespace eolo::bag {

class ZenohRecorderSubscribersManager {
public:
  using SubscriberCallback =
      std::function<void(const ipc::MessageMetadata& metadata, std::span<std::byte> message)>;

  ZenohRecorderSubscribersManager(ipc::zenoh::SessionPtr session, SubscriberCallback&& callback,
                                  TopicFilter topic_filter);

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  void createPublisherDiscovery();
  void onPublisher(const ipc::zenoh::PublisherInfo& info);
  void onPublisherAdded(const ipc::zenoh::PublisherInfo& info);
  void onPublisherDropped(const ipc::zenoh::PublisherInfo& info);

private:
  ipc::zenoh::SessionPtr session_;
  SubscriberCallback callback_;
  TopicFilter topic_filter_;

  std::unique_ptr<ipc::zenoh::PublisherDiscovery> discover_publishers_;

  mutable std::mutex subscribers_mutex_;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::Subscriber>>
      subscribers_ ABSL_GUARDED_BY(subscribers_mutex_);
};

// auto stop() -> std::future<void>;

}  // namespace eolo::bag
