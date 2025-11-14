//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributor
//=================================================================================================

#include "hephaestus/ipc/zenoh/dynamic_subscriber.h"

#include <cstddef>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/topic_database.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/telemetry/log/log.h"

namespace heph::ipc::zenoh {
// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved,-warnings-as-errors)
DynamicSubscriber::DynamicSubscriber(DynamicSubscriberParams&& params)
  : session_(std::move(params.session))
  , topic_filter_(ipc::TopicFilter::create(params.topics_filter_params))
  , topic_db_(createZenohTopicDatabase(session_))
  , init_subscriber_cb_(std::move(params.init_subscriber_cb))
  , subscriber_cb_(std::move(params.subscriber_cb)) {
}

[[nodiscard]] auto DynamicSubscriber::start() -> std::future<void> {
  discover_publishers_ = std::make_unique<EndpointDiscovery>(
      session_, topic_filter_, [this](const EndpointInfo& info) { onEndpointDiscovered(info); });

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

[[nodiscard]] auto DynamicSubscriber::stop() -> std::future<void> {
  discover_publishers_ = nullptr;
  subscribers_.erase(subscribers_.begin(), subscribers_.end());

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

void DynamicSubscriber::onEndpointDiscovered(const EndpointInfo& info) {
  if (info.type != EndpointType::PUBLISHER) {
    return;
  }

  onPublisher(info);
}

void DynamicSubscriber::onPublisher(const EndpointInfo& info) {
  switch (info.status) {
    case ipc::zenoh::EndpointInfo::Status::ALIVE:
      onPublisherAdded(info);
      break;
    case ipc::zenoh::EndpointInfo::Status::DROPPED:
      onPublisherDropped(info);
      break;
  }
}

void DynamicSubscriber::onPublisherAdded(const EndpointInfo& info) {
  if (subscribers_.contains(info.topic)) {
    heph::log(heph::ERROR, "trying to add subscriber for topic but one already exists", "topic", info.topic);
    return;
  }

  auto type_info = topic_db_->getTypeInfo(info.topic);
  if (!type_info) {
    // TODO(@fbrizzi): consider if we still want to allow for empty type info.
    heph::log(heph::ERROR, "failed to get type info for topic, skipping", "topic", info.topic);
    return;
  }

  if (init_subscriber_cb_) {
    init_subscriber_cb_(info.topic, type_info.value());
  }

  heph::log(heph::DEBUG, "create subscriber", "topic", info.topic);
  subscribers_[info.topic] = std::make_unique<ipc::zenoh::RawSubscriber>(
      session_, ipc::TopicConfig{ info.topic },
      [this, type_info = *type_info](const MessageMetadata& metadata, std::span<const std::byte> data) {
        subscriber_cb_(metadata, data, type_info);
      },
      *type_info);
}

void DynamicSubscriber::onPublisherDropped(const EndpointInfo& info) {
  if (!subscribers_.contains(info.topic)) {
    heph::log(heph::ERROR, "trying to drop subscriber, but one doesn't exist", "topic", info.topic);
    return;
  }

  heph::log(heph::DEBUG, "drop subscriber", "topic", info.topic);
  subscribers_[info.topic] = nullptr;
  subscribers_.extract(info.topic);
}

}  // namespace heph::ipc::zenoh
