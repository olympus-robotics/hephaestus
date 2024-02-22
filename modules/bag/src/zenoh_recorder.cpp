//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/zenoh_recorder.h"

#include <future>
#include <mutex>
#include <utility>

#include <absl/base/thread_annotations.h>
#include <fmt/core.h>

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
  void startRecordingBlocking();

private:
  std::unique_ptr<IBagWriter> bag_writer_;
  ZenohRecorderParams params_;

  containers::BlockingQueue<BagRecord> messages_;
  std::unique_ptr<ipc::zenoh::Subscriber> subscriber_;

  std::unique_ptr<ipc::ITopicDatabase> topic_db_;

  std::future<void> job_;

  std::unordered_map<std::string, std::once_flag> topic_flags_ ABSL_GUARDED_BY(flags_mutex_);
  std::mutex flags_mutex_;
};

ZenohRecorder::Impl::Impl(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params)
  : bag_writer_(std::move(writer))
  , params_(std::move(params))
  , messages_(params_.messages_queue_size)
  , topic_db_(ipc::createZenohTopicDatabase(params_.session)) {
}

auto ZenohRecorder::Impl::start() -> std::future<void> {
  createSubscriber();
  job_ = std::async(std::launch::async, [this]() { startRecordingBlocking(); });

  return std::async(std::launch::async, [] {});
}

auto ZenohRecorder::Impl::stop() -> std::future<void> {
  // Kill the subscriber.
  // Wait for all the data to be exhausted.
  return std::async(std::launch::async, [this] {
    // TODO: check that killing the subscriber actually stop receiving msgs.
    subscriber_ = nullptr;
    messages_.stop();
    job_.wait();
  });
}

void ZenohRecorder::Impl::createSubscriber() {
  // Here we leverage the fact that zenoh allows to subscribe to multiple topic via a single subscriber by
  // using wildcards.
  auto cb = [this](const ipc::MessageMetadata& metadata, std::span<const std::byte> buffer) {
    {
      // TODO: find a better way to do this.
      std::unique_lock<std::mutex> lock(flags_mutex_);  // This is need to protect topic_flags_.
      std::call_once(topic_flags_[metadata.topic], [&metadata]() {
        // TODO: replace with log.
        fmt::println("Recording topic: {}", metadata.topic);
      });
    }

    auto topic_info = topic_db_->getTypeInfo(metadata.topic);
    // TODO: copying the schema everytime is not efficient at all and should be optimized.
    // Ideally we want to call register schema once for the first topic and then avoid copying the schema.
    // To do this we need a DB to remember what we already registered.
    messages_.forceEmplace(RecordMetadata{ metadata, std::move(topic_info) },
                           std::vector<std::byte>{ buffer.begin(), buffer.end() });
  };

  subscriber_ = std::make_unique<ipc::zenoh::Subscriber>(params_.session, std::move(cb));
}

void ZenohRecorder::Impl::startRecordingBlocking() {
  while (true) {
    auto message = messages_.waitAndPop();
    if (!message) {
      break;
    }
    bag_writer_->writeRecord(message->first, message->second);
  }
}

// ----------------------------------------------------------------------------------------------------------

ZenohRecorder::ZenohRecorder(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params)
  : impl_(std::make_unique<Impl>(std::move(writer), std::move(params))) {
}

ZenohRecorder::~ZenohRecorder() = default;

auto ZenohRecorder::start() -> std::future<void> {
  return impl_->start();
}

auto ZenohRecorder::stop() -> std::future<void> {
  return impl_->stop();
}

auto ZenohRecorder::create(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params) -> ZenohRecorder {
  return { std::move(writer), std::move(params) };
}

}  // namespace eolo::bag
