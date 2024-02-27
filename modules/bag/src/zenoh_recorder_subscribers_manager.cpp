//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "zenoh_recorder_subscribers_manager.h"

#include <memory>

#include <absl/synchronization/mutex.h>

#include "eolo/base/exception.h"
#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/liveliness.h"
#include "eolo/ipc/zenoh/query.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/subscriber.h"
#include "gmock/gmock.h"

namespace eolo::bag {
ZenohRecorderSubscribersManager::ZenohRecorderSubscribersManager(ipc::zenoh::SessionPtr session,
                                                                 SubscriberCallback&& callback,
                                                                 TopicFilter topic_filter,
                                                                 IBagWriter& bag_writer)
  : session_(std::move(session))
  , callback_(std::move(callback))
  , topic_filter_(std::move(topic_filter))
  , bag_writer_(&bag_writer)
  , topic_info_query_session_(ipc::zenoh::createSession({}))
  , topic_db_(ipc::createZenohTopicDatabase(topic_info_query_session_)) {  // We create it with default
                                                                           // settings.
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
  auto type_info = topic_db_->getTypeInfo(info.topic);
  {
    absl::MutexLock lock{ &writer_mutex_ };
    bag_writer_->registerSchema(type_info);
    bag_writer_->registerChannel(info.topic, type_info);
  }

  {
    absl::MutexLock lock{ &subscribers_mutex_ };
    throwExceptionIf<InvalidOperationException>(
        subscribers_.contains(info.topic),
        std::format("adding subscriber for topic: {}, but one already exists", info.topic));

    auto cb = [this](const ipc::MessageMetadata& metadata, std::span<const std::byte> data) {
      // TODO: Hate that we need a mutex here, but the discovery needs to happen in a different session,
      // otherwise when looking for the type we may block.
      absl::MutexLock lock{ &writer_mutex_ };
      bag_writer_->writeRecord(metadata, data);
    };
    subscribers_[info.topic] = std::make_unique<ipc::zenoh::Subscriber>(session_, std::move(cb));
  }
}

void ZenohRecorderSubscribersManager::onPublisherDropped(const ipc::zenoh::PublisherInfo& info) {
  absl::MutexLock lock{ &subscribers_mutex_ };
  throwExceptionIf<InvalidOperationException>(
      !subscribers_.contains(info.topic),
      std::format("trying to stop recording from dropped topic {}, but subscriber doesn't exist",
                  info.topic));
  subscribers_[info.topic] = nullptr;
  subscribers_.extract(info.topic);
}
}  // namespace eolo::bag
