//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributor
//=================================================================================================

#pragma once

#include <future>

#include "hephaestus/ipc/topic_database.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc::zenoh {

using TopicWithTypeInfoCallback =
    std::function<void(const std::string& topic, const serdes::TypeInfo& type_info)>;
using SubscriberWithTypeCallback = std::function<void(const MessageMetadata&, std::span<const std::byte>,
                                                      const std::optional<serdes::TypeInfo>& type_info)>;

struct DynamicSubscriberParams {
  SessionPtr session;
  TopicFilterParams topics_filter_params;
  TopicWithTypeInfoCallback init_subscriber_cb;  /// This callback is called before creating a new subscriber
  SubscriberWithTypeCallback subscriber_cb;
};

/// This class actively listen for new publisher and for each new topic that passes the filter it creates a
/// new subscriber.
/// The user can provide a callback that is called once when a new publisher is discovered and
/// a callback to be passed to the topic subscriber.
class DynamicSubscriber {
public:
  explicit DynamicSubscriber(DynamicSubscriberParams&& params);

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  void onPublisher(const PublisherInfo& info);
  void onPublisherAdded(const PublisherInfo& info);
  void onPublisherDropped(const PublisherInfo& info);

private:
  SessionPtr session_;
  TopicFilter topic_filter_;

  std::unique_ptr<PublisherDiscovery> discover_publishers_;

  std::unique_ptr<ITopicDatabase> topic_db_;

  std::unordered_map<std::string, std::unique_ptr<RawSubscriber>> subscribers_;
  TopicWithTypeInfoCallback init_subscriber_cb_;
  SubscriberWithTypeCallback subscriber_cb_;
};

}  // namespace heph::ipc::zenoh
