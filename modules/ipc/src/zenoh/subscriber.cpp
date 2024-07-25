//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/subscriber.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <zenoh.h>
#include <zenoh_macros.h>
#include <zenohc.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {
Subscriber::Subscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
                       bool dedicated_callback_thread /*= false*/)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , dedicated_callback_thread_(dedicated_callback_thread) {
  zenohc::ClosureSample cb = [this](const zenohc::Sample& sample) { this->callback(sample); };
  if (session_->config.cache_size == 0) {
    const auto& topic = topic_config_.name;
    subscriber_ = expectAsUniquePtr(session_->zenoh_session.declare_subscriber(topic, std::move(cb)));
  } else {
    const auto sub_opts = ze_querying_subscriber_options_default();
    auto c = cb.take();
    cache_subscriber_ = ze_declare_querying_subscriber(
        session_->zenoh_session.loan(), z_keyexpr(topic_config_.name.c_str()), z_move(c), &sub_opts);
    heph::throwExceptionIf<heph::FailedZenohOperation>(!z_check(cache_subscriber_),
                                                       "failed to create zenoh sub");
  }

  if (dedicated_callback_thread_) {
    callback_messages_consumer_ = std::make_unique<concurrency::MessageQueueConsumer<Message>>(
        [this](const Message& message) {
          const auto& [metadata, buffer] = message;
          callback_(metadata, std::span<const std::byte>(buffer.begin(), buffer.end()));
        },
        DEFAULT_CACHE_RESERVES);
    callback_messages_consumer_->start();
  }
}

Subscriber::~Subscriber() {
  if (callback_messages_consumer_ != nullptr) {
    callback_messages_consumer_->stop();
  }

  z_drop(z_move(cache_subscriber_));
}

void Subscriber::callback(const zenohc::Sample& sample) {
  MessageMetadata metadata;
  if (sample.get_attachment().check()) {
    auto sequence_id = std::string{ sample.get_attachment().get(messageCounterKey()).as_string_view() };
    metadata.sequence_id = std::stoul(sequence_id);
    metadata.sender_id = sample.get_attachment().get(sessionIdKey()).as_string_view();
  }

  metadata.timestamp = toChrono(sample.get_timestamp());
  metadata.topic = sample.get_keyexpr().as_string_view();

  auto buffer = toByteSpan(sample.get_payload());
  if (dedicated_callback_thread_) {
    callback_messages_consumer_->queue().forceEmplace(metadata,
                                                      std::vector<std::byte>(buffer.begin(), buffer.end()));
  } else {
    callback_(metadata, buffer);
  }
}

}  // namespace heph::ipc::zenoh
