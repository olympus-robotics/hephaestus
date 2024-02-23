//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <future>
#include <span>

#include "eolo/bag/topic_filter.h"
#include "eolo/ipc/common.h"

namespace eolo::bag {

class ZenohRecorderSubscribersManager {
public:
  using SubscriberCallback =
      std::function<void(const ipc::MessageMetadata& metadata, std::span<std::byte> message)>;

  ZenohRecorderSubscribersManager(SubscriberCallback&& callback, TopicFilter topic_filter);

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  ZenohRecorderSubscribersManager();

private:
  SubscriberCallback callback_;
  TopicFilter topic_filter_;
};

ZenohRecorderSubscribersManager::ZenohRecorderSubscribersManager(SubscriberCallback&& callback,
                                                                 TopicFilter topic_filter)
  : callback_(std::move(callback), topic_filter_(std::move(topic_filter))) {
}

auto ZenohRecorderSubscribersManager::start() -> std::future<void> {
}
// auto stop() -> std::future<void>;

}  // namespace eolo::bag
