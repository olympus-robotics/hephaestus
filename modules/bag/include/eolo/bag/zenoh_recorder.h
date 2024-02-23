//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <future>

#include "eolo/bag/writer.h"
#include "eolo/ipc/zenoh/session.h"

namespace eolo::bag {

struct TopicsFilterParams {
  std::vector<std::string> include_topics_only;  //!< If specified only the topics in this list are
                                                 //!< going to be recorded. This rule has precedence
                                                 //!< over all the other
  std::string prefix;                            //!< Record all the topic sharing the prefix
  std::vector<std::string> exclude_topics;       //!< List of topics to exclude
};

struct ZenohRecorderParams {
  // Something to fileter topic to record.
  static constexpr size_t DEFAULT_MESSAGES_QUEUE_SIZE = 1e2;
  size_t messages_queue_size{ DEFAULT_MESSAGES_QUEUE_SIZE };

  ipc::zenoh::SessionPtr session;

  TopicsFilterParams topics_filter_params;
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
