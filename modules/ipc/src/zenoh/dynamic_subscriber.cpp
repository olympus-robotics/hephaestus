//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributor
//=================================================================================================

#include "hephaestus/ipc/zenoh/dynamic_subscriber.h"

#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <utility>

#include <absl/log/log.h>
#include <fmt/core.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/topic_database.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved,-warnings-as-errors)
DynamicSubscriber::DynamicSubscriber(DynamicSubscriberParams&& params)
  : session_(std::move(params.session))
  , topic_filter_(ipc::TopicFilter::create(params.topics_filter_params))
  , publisher_discovery_session_(zenoh::createSession(Config{ session_->config }))
  , topic_info_query_session_(ipc::zenoh::createSession({ Config{ session_->config } }))
  , topic_db_(createZenohTopicDatabase(topic_info_query_session_))
  , init_subscriber_cb_(std::move(params.init_subscriber_cb))
  , subscriber_cb_(std::move(params.subscriber_cb)) {
}

[[nodiscard]] auto DynamicSubscriber::start() -> std::future<void> {
  discover_publishers_ =
      std::make_unique<PublisherDiscovery>(publisher_discovery_session_, TopicConfig{ .name = "**" },
                                           [this](const PublisherInfo& info) { onPublisher(info); });

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

[[nodiscard]] auto DynamicSubscriber::stop() -> std::future<void> {
  discover_publishers_ = nullptr;
  {
    const std::scoped_lock lock(subscribers_mutex_);
    subscribers_.erase(subscribers_.begin(), subscribers_.end());
  }
  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

void DynamicSubscriber::onPublisher(const PublisherInfo& info) {
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

void DynamicSubscriber::onPublisherAdded(const PublisherInfo& info) {
  std::optional<serdes::TypeInfo> optional_type_info;
  if (init_subscriber_cb_) {
    auto type_info = topic_db_->getTypeInfo(info.topic);
    init_subscriber_cb_(info.topic, type_info);
    optional_type_info = std::move(type_info);
  }

  const std::scoped_lock lock(subscribers_mutex_);
  if (subscribers_.contains(info.topic)) {
    LOG(ERROR) << fmt::format("Adding subscriber for topic: {}, but one already exists", info.topic);
    return;
  }

  LOG(INFO) << fmt::format("Create subscriber for topic: {}", info.topic);
  subscribers_[info.topic] = std::make_unique<ipc::zenoh::RawSubscriber>(
      session_, ipc::TopicConfig{ .name = info.topic },
      [this, optional_type_info = std::move(optional_type_info)](const MessageMetadata& metadata,
                                                                 std::span<const std::byte> data) {
        subscriber_cb_(metadata, data, optional_type_info);
      });
}

void DynamicSubscriber::onPublisherDropped(const PublisherInfo& info) {
  const std::scoped_lock lock(subscribers_mutex_);
  if (!subscribers_.contains(info.topic)) {
    LOG(ERROR) << fmt::format("Dropping subscriber for topic: {}, but one doesn't exist", info.topic);
    return;
  }

  LOG(INFO) << fmt::format("Drop subscriber for topic: {}", info.topic);
  subscribers_[info.topic] = nullptr;
  subscribers_.extract(info.topic);
}

}  // namespace heph::ipc::zenoh
