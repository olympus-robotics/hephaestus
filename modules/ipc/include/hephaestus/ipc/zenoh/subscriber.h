//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include <zenoh.h>
#include <zenoh.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

class Subscriber {
public:
  using DataCallback = std::function<void(const MessageMetadata&, std::span<const std::byte>)>;

  /// Note: setting dedicated_callback_thread to true will consume the messages in a dedicated thread.
  /// While this avoid blocking the Zenoh session thread to process other messages,
  /// it also introduce an overhead due to the message data being copied.
  Subscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
             bool dedicated_callback_thread = false);
  ~Subscriber();
  Subscriber(const Subscriber&) = delete;
  Subscriber(Subscriber&&) = delete;
  auto operator=(const Subscriber&) -> Subscriber& = delete;
  auto operator=(Subscriber&&) -> Subscriber& = delete;

private:
  void callback(const ::zenoh::Sample& sample);

private:
  using Message = std::pair<MessageMetadata, std::vector<std::byte>>;

  SessionPtr session_;
  TopicConfig topic_config_;

  DataCallback callback_;

  std::unique_ptr<::zenoh::Subscriber<void>> subscriber_;

  bool enable_cache_ = false;
  ze_owned_querying_subscriber_t cache_subscriber_{};
  z_owned_session_t zenoh_cache_session_{};

  bool dedicated_callback_thread_;
  static constexpr std::size_t DEFAULT_CACHE_RESERVES = 100;
  std::unique_ptr<concurrency::MessageQueueConsumer<Message>> callback_messages_consumer_;
};

}  // namespace heph::ipc::zenoh
