//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/zenoh_recorder.h"

#include <future>
#include <memory>
#include <utility>

#include <absl/base/thread_annotations.h>
#include <absl/log/log.h>
#include <absl/synchronization/mutex.h>
#include <fmt/core.h>

#include "eolo/bag/topic_filter.h"
#include "eolo/bag/writer.h"
#include "eolo/ipc/topic_database.h"
#include "eolo/ipc/zenoh/liveliness.h"
#include "eolo/ipc/zenoh/subscriber.h"

namespace eolo::bag {

using BagRecord = std::pair<ipc::MessageMetadata, std::vector<std::byte>>;

class ZenohRecorder::Impl {
public:
  explicit Impl(ZenohRecorderParams params);

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  void onPublisher(const ipc::zenoh::PublisherInfo& info);
  void onPublisherAdded(const ipc::zenoh::PublisherInfo& info);
  void onPublisherDropped(const ipc::zenoh::PublisherInfo& info);

private:
  std::unique_ptr<IBagWriter> bag_writer_ ABSL_GUARDED_BY(writer_mutex_);
  absl::Mutex writer_mutex_;

  TopicFilter topic_filter_;

  ipc::zenoh::SessionPtr session_;
  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::Subscriber>>
      subscribers_ ABSL_GUARDED_BY(subscribers_mutex_);
  absl::Mutex subscribers_mutex_;

  ipc::zenoh::SessionPtr topic_info_query_session_;  // Session used to query topic service.
  std::unique_ptr<ipc::ITopicDatabase> topic_db_;

  ipc::zenoh::SessionPtr publisher_discovery_session_;
  std::unique_ptr<ipc::zenoh::PublisherDiscovery> discover_publishers_;
};

ZenohRecorder::Impl::Impl(ZenohRecorderParams params)
  : bag_writer_(std::move(params.bag_writer))
  , topic_filter_(TopicFilter::create(params.topics_filter_params))
  , session_(std::move(params.session))
  , topic_info_query_session_(ipc::zenoh::createSession({ .topic = "**" }))
  , topic_db_(ipc::createZenohTopicDatabase(topic_info_query_session_))
  , publisher_discovery_session_(ipc::zenoh::createSession({ .topic = "**" })) {
}

auto ZenohRecorder::Impl::start() -> std::future<void> {
  discover_publishers_ = std::make_unique<ipc::zenoh::PublisherDiscovery>(
      publisher_discovery_session_, [this](const ipc::zenoh::PublisherInfo& info) { onPublisher(info); });

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

auto ZenohRecorder::Impl::stop() -> std::future<void> {
  discover_publishers_ = nullptr;
  subscribers_.erase(subscribers_.begin(), subscribers_.end());

  std::promise<void> promise;
  promise.set_value();
  return promise.get_future();
}

void ZenohRecorder::Impl::onPublisher(const ipc::zenoh::PublisherInfo& info) {
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

void ZenohRecorder::Impl::onPublisherAdded(const ipc::zenoh::PublisherInfo& info) {
  auto type_info = topic_db_->getTypeInfo(info.topic);
  {
    absl::MutexLock lock{ &writer_mutex_ };
    bag_writer_->registerSchema(type_info);
    bag_writer_->registerChannel(info.topic, type_info);
  }

  auto cb = [this](const ipc::MessageMetadata& metadata, std::span<const std::byte> data) {
    // TODO: Hate that we need a mutex here, but the discovery needs to happen in a different session,
    // otherwise when looking for the type we may block.
    absl::MutexLock lock{ &writer_mutex_ };
    bag_writer_->writeRecord(metadata, data);
  };

  {
    absl::MutexLock lock{ &subscribers_mutex_ };
    throwExceptionIf<InvalidOperationException>(
        subscribers_.contains(info.topic),
        std::format("adding subscriber for topic: {}, but one already exists", info.topic));

    LOG(INFO) << std::format("Create subscriber for topic: {}", info.topic);
    // TODO: I don't like this at all I need to decouple the topic from the session
    session_->config.topic = info.topic;
    subscribers_[info.topic] = std::make_unique<ipc::zenoh::Subscriber>(session_, std::move(cb));
  }
}

void ZenohRecorder::Impl::onPublisherDropped(const ipc::zenoh::PublisherInfo& info) {
  absl::MutexLock lock{ &subscribers_mutex_ };
  throwExceptionIf<InvalidOperationException>(
      !subscribers_.contains(info.topic),
      std::format("trying to stop recording from dropped topic {}, but subscriber doesn't exist",
                  info.topic));
  subscribers_[info.topic] = nullptr;
  subscribers_.extract(info.topic);
  LOG(INFO) << std::format("Drop subscriber for topic: {}", info.topic);
}

// ----------------------------------------------------------------------------------------------------------

ZenohRecorder::ZenohRecorder(ZenohRecorderParams params) : impl_(std::make_unique<Impl>(std::move(params))) {
}

ZenohRecorder::~ZenohRecorder() = default;

auto ZenohRecorder::start() -> std::future<void> {
  return impl_->start();
}

auto ZenohRecorder::stop() -> std::future<void> {
  return impl_->stop();
}

auto ZenohRecorder::create(ZenohRecorderParams params) -> ZenohRecorder {
  return ZenohRecorder{ std::move(params) };
}

}  // namespace eolo::bag
