//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "zenoh_recorder_subscribers_manager.h"

#include <memory>

#include "eolo/ipc/zenoh/liveliness.h"
#include "eolo/ipc/zenoh/session.h"
namespace eolo::bag {
ZenohRecorderSubscribersManager::ZenohRecorderSubscribersManager(ipc::zenoh::SessionPtr session,
                                                                 SubscriberCallback&& callback,
                                                                 TopicFilter topic_filter)
  : session_(std::move(session)), callback_(std::move(callback)), topic_filter_(std::move(topic_filter)) {
}

auto ZenohRecorderSubscribersManager::start() -> std::future<void> {
  createPublisherDiscovery();

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

void ZenohRecorderSubscribersManager::createPublisherDiscovery() {
  auto cb = [this](const ipc::zenoh::PublisherInfo& info) { onPublisher(info); };
  discover_publishers_ = std::make_unique<ipc::zenoh::PublisherDiscovery>(session_, std::move(cb));
}

void ZenohRecorderSubscribersManager::onPublisher(const ipc::zenoh::PublisherInfo& info) {
  if (!topic_filter_.isAcceptable(info.topic)) {
    return;
  }

  switch (info.status) {
    case ipc::zenoh::PublisherStatus::ALIVE:
      onPublisherAdded(info);
      break;
    case ipc::zenoh::PublisherStatus::DROPPED:
      onPublisherDropped(info);
      break;
  }
}

void ZenohRecorderSubscribersManager::onPublisherAdded(const ipc::zenoh::PublisherInfo& info) {
  (void)info;
}

void ZenohRecorderSubscribersManager::onPublisherDropped(const ipc::zenoh::PublisherInfo& info) {
  (void)info;
}
}  // namespace eolo::bag
