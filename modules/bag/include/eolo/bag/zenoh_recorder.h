//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <future>

#include "eolo/bag/writer.h"
#include "eolo/ipc/zenoh/session.h"

namespace eolo::bag {

struct ZenohRecorderParams {
  // Something to fileter topic to record.
  static constexpr size_t DEFAULT_MESSAGES_QUEUE_SIZE = 1e2;
  size_t messages_queue_size{ DEFAULT_MESSAGES_QUEUE_SIZE };

  std::string topic;

  ipc::zenoh::SessionPtr session;
};

class ZenohRecorder {
public:
  ~ZenohRecorder();

  [[nodiscard]] static auto create(std::unique_ptr<IBagWriter> writer,
                                   ZenohRecorderParams params) -> ZenohRecorder;

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  ZenohRecorder(std::unique_ptr<IBagWriter> writer, ZenohRecorderParams params);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace eolo::bag
