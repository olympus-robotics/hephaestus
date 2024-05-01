//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <future>

#include "hephaestus/bag/writer.h"
#include "hephaestus/ipc/topic_filter.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::bag {

struct ZenohRecorderParams {
  ipc::zenoh::SessionPtr session;
  std::unique_ptr<IBagWriter> bag_writer;
  ipc::TopicFilterParams topics_filter_params;
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

}  // namespace heph::bag
