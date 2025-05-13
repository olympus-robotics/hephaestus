//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <zenoh/api/ext/advanced_subscriber.hxx>
#include <zenoh/api/liveliness.hxx>
#include <zenoh/api/sample.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc::zenoh {

struct MessageMetadata {
  std::string sender_id;
  std::string topic;
  std::string type_info;
  std::chrono::nanoseconds timestamp{};
  std::size_t sequence_id{};
};

struct SubscriberConfig {
  std::optional<std::size_t> cache_size{ std::nullopt };
  bool dedicated_callback_thread{ false };
  bool create_liveliness_token{ true };
  bool create_type_info_service{ true };
};

class RawSubscriber {
public:
  using DataCallback = std::function<void(const MessageMetadata&, std::span<const std::byte>)>;

  /// Note: setting dedicated_callback_thread to true will consume the messages in a dedicated thread.
  /// While this avoid blocking the Zenoh session thread to process other messages,
  /// it also introduce an overhead due to the message data being copied.
  RawSubscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
                serdes::TypeInfo type_info, const SubscriberConfig& config = {});
  ~RawSubscriber();
  RawSubscriber(const RawSubscriber&) = delete;
  RawSubscriber(RawSubscriber&&) = delete;
  auto operator=(const RawSubscriber&) -> RawSubscriber& = delete;
  auto operator=(RawSubscriber&&) -> RawSubscriber& = delete;

private:
  void callback(const ::zenoh::Sample& sample);
  void createTypeInfoService();

private:
  using Message = std::pair<MessageMetadata, std::vector<std::byte>>;

  SessionPtr session_;
  TopicConfig topic_config_;

  DataCallback callback_;

  std::unique_ptr<::zenoh::ext::AdvancedSubscriber<void>> subscriber_;
  std::unique_ptr<::zenoh::LivelinessToken> liveliness_token_;

  serdes::TypeInfo type_info_;
  std::unique_ptr<Service<std::string, std::string>> type_service_;

  bool dedicated_callback_thread_;
  static constexpr std::size_t DEFAULT_CACHE_RESERVES = 100;
  std::unique_ptr<concurrency::MessageQueueConsumer<Message>> callback_messages_consumer_;
};

}  // namespace heph::ipc::zenoh
