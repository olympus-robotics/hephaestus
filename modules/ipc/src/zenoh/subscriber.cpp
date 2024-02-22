//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/subscriber.h"

#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {
Subscriber::Subscriber(SessionPtr session, DataCallback&& callback)
  : session_(std::move(session)), callback_(std::move(callback)) {
  zenohc::ClosureSample cb = [this](const zenohc::Sample& sample) { this->callback(sample); };
  if (session->config.cache_size == 0) {
    subscriber_ =
        expectAsUniquePtr(session_->zenoh_session.declare_subscriber(session->config.topic, std::move(cb)));
  } else {
    const auto sub_opts = ze_querying_subscriber_options_default();
    auto c = cb.take();
    cache_subscriber_ = ze_declare_querying_subscriber(
        session_->zenoh_session.loan(), z_keyexpr(session->config.topic.c_str()), z_move(c), &sub_opts);
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
