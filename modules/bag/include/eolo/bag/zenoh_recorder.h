//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <future>

#include "eolo/bag/topic_filter.h"
#include "eolo/bag/writer.h"
#include "eolo/ipc/zenoh/session.h"

namespace eolo::bag {

struct ZenohRecorderParams {
  ipc::zenoh::SessionPtr session;
  std::unique_ptr<IBagWriter> bag_writer;
  TopicFilterParams topics_filter_params;
};

class ZenohRecorder {
public:
  ~ZenohRecorder();

  [[nodiscard]] static auto create(ZenohRecorderParams params) -> ZenohRecorder;

  [[nodiscard]] auto start() -> std::future<void>;

  [[nodiscard]] auto stop() -> std::future<void>;

private:
  explicit ZenohRecorder(ZenohRecorderParams params);

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace eolo::bag
