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
#include <zenoh.hxx>
#include <zenoh_macros.h>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {
namespace {
// This code comes from `zenoh/api/session.hxx`
template <typename C, typename D>
[[nodiscard]] auto createZenohcClosure(C&& on_sample, D&& on_drop) -> z_owned_closure_sample_t {
  z_owned_closure_sample_t c_closure;
  using Cval = std::remove_reference_t<C>;
  using Dval = std::remove_reference_t<D>;
  using ClosureType = typename ::zenoh::detail::closures::Closure<Cval, Dval, void, const ::zenoh::Sample&>;
  auto closure = ClosureType::into_context(std::forward<C>(on_sample), std::forward<D>(on_drop));
  ::z_closure(&c_closure, ::zenoh::detail::closures::_zenoh_on_sample_call,
              ::zenoh::detail::closures::_zenoh_on_drop, closure);
}
}  // namespace

Subscriber::Subscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
                       bool dedicated_callback_thread /*= false*/)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , dedicated_callback_thread_(dedicated_callback_thread) {
  auto cb = [this](const ::zenoh::Sample& sample) { this->callback(sample); };
  if (session_->config.cache_size == 0) {
    const auto& topic = topic_config_.name;
    subscriber_ = std::make_unique<::zenoh::Subscriber<void>>(
        session_->zenoh_session.declare_subscriber(topic, std::move(cb), ::zenoh::closures::none));
  } else {
    // zenohcxx still doesn't support cache querying subscribers, so we have to use the C API.
    ze_querying_subscriber_options_t sub_opts;
    ze_querying_subscriber_options_default(&sub_opts);

    z_view_keyexpr_t ke;
    z_view_keyexpr_from_str(&ke, topic_config_.name.c_str());

    auto c_closure = createZenohcClosure(cb, ::zenoh::closures::none);

    auto new_session = createSession(session_->config);
    zenoh_session_ = std::move(new_session->zenoh_session).take();
    auto success = ze_declare_querying_subscriber(&cache_subscriber_, z_loan(zenoh_session_), z_loan(ke),
                                                  z_move(c_closure), &sub_opts);

    heph::throwExceptionIf<heph::FailedZenohOperation>(success != Z_OK, "failed to create zenoh sub");
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
  z_drop(z_move(zenoh_session_));
}

void Subscriber::callback(const ::zenoh::Sample& sample) {
  MessageMetadata metadata;
  if (const auto attachment = sample.get_attachment(); attachment.has_value()) {
    auto attachment_data = attachment->get().deserialize<std::unordered_map<std::string, std::string>>();
    metadata.sequence_id = std::stoul(attachment_data[messageCounterKey()]);
    metadata.sender_id = attachment_data[sessionIdKey()];
  }

  if (const auto timestamp = sample.get_timestamp(); timestamp.has_value()) {
    metadata.timestamp = toChrono(timestamp.value());
  }
  metadata.topic = sample.get_keyexpr().as_string_view();

  auto buffer = sample.get_payload().deserialize<std::vector<std::byte>>();
  if (dedicated_callback_thread_) {
    callback_messages_consumer_->queue().forceEmplace(metadata, buffer);
  } else {
    callback_(metadata, buffer);
  }
}

}  // namespace heph::ipc::zenoh
