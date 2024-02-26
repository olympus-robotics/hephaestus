//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "zenoh_recorder_subscribers_manager.h"

#include <memory>

#include "eolo/base/exception.h"
#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/liveliness.h"
#include "eolo/ipc/zenoh/query.h"
#include "eolo/ipc/zenoh/session.h"
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
  , query_session_(ipc::zenoh::createSession({}))
  , topic_db_(ipc::createZenohTopicDatabase(query_session_)) {  // We create it with default settings.
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
  // This callback happen on the same session as the subscribers so it's sequential.
  // Is this going to be a problem?!?!

  auto service_topic = ipc::getTypeInfoServiceTopic(info.topic);
  auto topic_type_info = ipc::zenoh::query(query_session_->zenoh_session, service_topic, "");
  throwExceptionIf<InvalidDataException>(
      topic_type_info.size() != 1, std::format("expect 1 response from service on topic: {}, received {}",
                                               info.topic, topic_type_info.size()));
  const auto& topic = topic_type_info.front().topic;
  auto type_info = topic_db_->getTypeInfo(topic);
  bag_writer_->registerSchema(type_info);
  bag_writer_->registerChannel(topic, type_info);

  // writer register schema and channel
  // Create new subscriber
}

void ZenohRecorderSubscribersManager::onPublisherDropped(const ipc::zenoh::PublisherInfo& info) {
  std::unique_lock<std::mutex> lock{ subscribers_mutex_ };
  throwExceptionIf<InvalidOperationException>(
      !subscribers_.contains(info.topic),
      std::format("trying to stop recording from dropped topic {}, but subscriber doesn't exist",
                  info.topic));
  subscribers_[info.topic] = nullptr;
  subscribers_.extract(info.topic);
}
}  // namespace eolo::bag
