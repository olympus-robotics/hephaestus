//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/bag/zenoh_recorder.h"

#include <cstddef>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>

#include "hephaestus/bag/writer.h"
#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/dynamic_subscriber.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::bag {

using BagRecord = std::pair<ipc::MessageMetadata, std::vector<std::byte>>;

/// This class does the following:
/// - constantly checks for new topics
/// - for each new topic, check if it passes the recording filter
/// - query topic service to get topic type and register the new schema and channel with the bag writer
/// - create a new subscriber to it
/// This class uses 3 different sessions:
/// - to subscribe to the topics
/// - for the topic service query
/// - for the topic discovery
/// the reason we need three sessions is to allow parallel execution of callback as for one session all
/// callbacks happen sequentially.
class ZenohRecorder::Impl {
public:
  explicit Impl(ZenohRecorderParams params);

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  std::unique_ptr<IBagWriter> bag_writer_ ABSL_GUARDED_BY(writer_mutex_);
  absl::Mutex writer_mutex_;

  std::unique_ptr<ipc::zenoh::DynamicSubscriber> dynamic_subscriber_;
};

ZenohRecorder::Impl::Impl(ZenohRecorderParams params)
  : bag_writer_(std::move(params.bag_writer))
  , dynamic_subscriber_(std::make_unique<ipc::zenoh::DynamicSubscriber>(ipc::zenoh::DynamicSubscriberParams{
        .session = std::move(params.session),
        .topics_filter_params = std::move(params.topics_filter_params),
        .init_subscriber_cb =
            [this](const std::string& topic, const serdes::TypeInfo& type_info) {
              const absl::MutexLock lock{ &writer_mutex_ };
              bag_writer_->registerSchema(type_info);
              bag_writer_->registerChannel(topic, type_info);
            },
        .subscriber_cb =
            [this](const ipc::MessageMetadata& metadata, std::span<const std::byte> data,
                   const std::optional<serdes::TypeInfo>& type_info) {
              (void)type_info;
              const absl::MutexLock lock{ &writer_mutex_ };
              bag_writer_->writeRecord(metadata, data);
            },
    })) {
}

auto ZenohRecorder::Impl::start() -> std::future<void> {
  return dynamic_subscriber_->start();
}

auto ZenohRecorder::Impl::stop() -> std::future<void> {
  return dynamic_subscriber_->stop();
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

}  // namespace heph::bag
