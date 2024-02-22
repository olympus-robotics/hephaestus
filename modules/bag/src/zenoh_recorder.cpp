//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/zenoh_recorder.h"

#include <future>
#include <utility>

#include "eolo/bag/writer.h"
#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/subscriber.h"

namespace eolo::bag {

using BagRecord = std::pair<ipc::MessageMetadata, std::vector<std::byte>>;

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

  // containers::BlockingQueue<BagRecord> messages_;
  std::unique_ptr<ipc::zenoh::Subscriber> subscriber_;
};

ZenohRecorder::Impl::Impl(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params)
  : writer_(std::move(writer)), params_(std::move(params)) {
}

auto ZenohRecorder::Impl::start() -> std::future<void> {
  createSubscriber();

  return std::async(std::launch::async, [] {});
}

auto ZenohRecorder::Impl::stop() -> std::future<void> {
  return std::async(std::launch::async, [] {});
}

void ZenohRecorder::Impl::createSubscriber() {
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
