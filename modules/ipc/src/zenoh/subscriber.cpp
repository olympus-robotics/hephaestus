//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/subscriber.h"

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {
Subscriber::Subscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback)
  : session_(std::move(session)), topic_config_(std::move(topic_config)), callback_(std::move(callback)) {
  zenohc::ClosureSample cb = [this](const zenohc::Sample& sample) { this->callback(sample); };
  if (session_->config.cache_size == 0) {
    const auto& topic = topic_config_.name;
    subscriber_ = expectAsUniquePtr(session_->zenoh_session.declare_subscriber(topic, std::move(cb)));
  } else {
    const auto sub_opts = ze_querying_subscriber_options_default();
    auto c = cb.take();
    cache_subscriber_ = ze_declare_querying_subscriber(
        session_->zenoh_session.loan(), z_keyexpr(topic_config_.name.c_str()), z_move(c), &sub_opts);
    eolo::throwExceptionIf<eolo::FailedZenohOperation>(!z_check(cache_subscriber_),
                                                       "failed to create zenoh sub");
  }
}

Subscriber::~Subscriber() {
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
  callback_(metadata, buffer);
}

}  // namespace eolo::ipc::zenoh
