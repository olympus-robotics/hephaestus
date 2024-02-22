//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/zenoh_recorder.h"

#include <future>
#include <utility>

#include "eolo/bag/writer.h"
#include "eolo/containers/blocking_queue.h"
#include "eolo/ipc/topic_database.h"
#include "eolo/ipc/zenoh/subscriber.h"

namespace eolo::bag {

using BagRecord = std::pair<RecordMetadata, std::vector<std::byte>>;

class ZenohRecorder::Impl {
public:
  Impl(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params);

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  void createSubscriber();

private:
  std::unique_ptr<IBagWriter> writer_;
  ZenohRecorderParams params_;

  containers::BlockingQueue<BagRecord> messages_;
  std::unique_ptr<ipc::zenoh::Subscriber> subscriber_;

  std::unique_ptr<ipc::ITopicDatabase> topic_db_;
};

ZenohRecorder::Impl::Impl(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params)
  : writer_(std::move(writer))
  , params_(std::move(params))
  , messages_(params_.messages_queue_size)
  , topic_db_(ipc::createZenohTopicDatabase(params_.session)) {
}

auto ZenohRecorder::Impl::start() -> std::future<void> {
  createSubscriber();

  return std::async(std::launch::async, [] {});
}

auto ZenohRecorder::Impl::stop() -> std::future<void> {
  // Kill the subscriber.
  // Wait for all the data to be exhausted.
  return std::async(std::launch::async, [] {});
}

void ZenohRecorder::Impl::createSubscriber() {
  // Here we leverage the fact that zenoh allows to subscribe to multiple topic via a single subscriber by
  // using wildcards.
}

// ---------------------

ZenohRecorder::ZenohRecorder(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params)
  : impl_(std::make_unique<ZenohRecorder::Impl>(std::move(writer), params)) {
}

auto ZenohRecorder::start() -> std::future<void> {
  return impl_->start();
}

auto ZenohRecorder::stop() -> std::future<void> {
  return impl_->stop();
}

}  // namespace eolo::bag
