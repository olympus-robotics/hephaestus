//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/raw_subscriber.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <absl/strings/numbers.h>
#include <fmt/format.h>
#include <zenoh.h>
#include <zenoh/api/base.hxx>
#include <zenoh/api/closures.hxx>
#include <zenoh/api/ext/serialization.hxx>
#include <zenoh/api/interop.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/api/sample.hxx>
#include <zenoh/api/subscriber.hxx>
#include <zenoh/detail/closures.hxx>
#include <zenoh/detail/closures_concrete.hxx>
#include <zenoh_macros.h>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/telemetry/log.h"
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
  return c_closure;
}

[[nodiscard]] auto getMetadata(const ::zenoh::Sample& sample, const std::string& topic) -> MessageMetadata {
  std::string sender_id;
  std::string type_info;
  std::size_t sequence_id{};
  if (const auto attachment = sample.get_attachment(); attachment.has_value()) {
    auto attachment_data =
        ::zenoh::ext::deserialize<std::unordered_map<std::string, std::string>>(attachment->get());

    auto res = absl::SimpleAtoi(attachment_data[PUBLISHER_ATTACHMENT_MESSAGE_COUNTER_KEY], &sequence_id);
    heph::logIf(heph::ERROR, !res, "failed to read message counter from attachment", "service", topic);

    sender_id = attachment_data[PUBLISHER_ATTACHMENT_MESSAGE_SESSION_ID_KEY];
    type_info = attachment_data[PUBLISHER_ATTACHMENT_MESSAGE_TYPE_INFO];
  }

  auto timestamp = std::chrono::nanoseconds{ 0 };
  if (const auto zenoh_timestamp = sample.get_timestamp(); zenoh_timestamp.has_value()) {
    timestamp = toChrono(zenoh_timestamp.value());
  }

  return {
    .sender_id = std::move(sender_id),
    .topic = std::string{ sample.get_keyexpr().as_string_view() },
    .type_info = std::move(type_info),
    .timestamp = timestamp,
    .sequence_id = sequence_id,
  };
}
}  // namespace

RawSubscriber::RawSubscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
                             bool dedicated_callback_thread /*= false*/)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , enable_cache_(session_->config.cache_size > 0)
  , dedicated_callback_thread_(dedicated_callback_thread) {
  auto cb = [this](const ::zenoh::Sample& sample) { this->callback(sample); };
  if (!enable_cache_) {
    ::zenoh::ZResult result{};
    const ::zenoh::KeyExpr keyexpr{ topic_config_.name };
    subscriber_ = std::make_unique<::zenoh::Subscriber<void>>(session_->zenoh_session.declare_subscriber(
        keyexpr, std::move(cb), ::zenoh::closures::none,
        ::zenoh::Session::SubscriberOptions::create_default(), &result));
    heph::throwExceptionIf<heph::FailedZenohOperation>(
        result != Z_OK,
        fmt::format("[Subscriber {}] failed to create zenoh subscriber, err {}", topic_config_.name, result));
  } else {
    // zenohcxx still doesn't support cache querying subscribers, so we have to use the C API.
    ze_querying_subscriber_options_t sub_opts;
    ze_querying_subscriber_options_default(&sub_opts);

    z_view_keyexpr_t keyexpr;
    z_view_keyexpr_from_str(&keyexpr, topic_config_.name.c_str());

    auto c_closure = createZenohcClosure(cb, ::zenoh::closures::none);

    const auto result =
        ze_declare_querying_subscriber(::zenoh::interop::as_loaned_c_ptr(session_->zenoh_session),
                                       &cache_subscriber_, z_loan(keyexpr), z_move(c_closure), &sub_opts);

    heph::throwExceptionIf<heph::FailedZenohOperation>(
        result != Z_OK,
        fmt::format("[Subscriber {}] failed to create zenoh subscriber, err {}", topic_config_.name, result));
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

// NOLINTNEXTLINE(bugprone-exception-escape)
RawSubscriber::~RawSubscriber() {
  if (callback_messages_consumer_ != nullptr) {
    auto stopped = callback_messages_consumer_->stop();
    stopped.get();
  }

  if (enable_cache_) {
    z_drop(z_move(cache_subscriber_));
  } else {
    try {
      std::move(*subscriber_).undeclare();
    } catch (std::exception& e) {
      heph::log(heph::ERROR, "failed to undeclare subscriber", "topic", topic_config_.name, "exception",
                e.what());
    }
  }
}

void RawSubscriber::callback(const ::zenoh::Sample& sample) {
  const auto metadata = getMetadata(sample, topic_config_.name);
  auto payload = toByteVector(sample.get_payload());

  if (dedicated_callback_thread_) {
    callback_messages_consumer_->queue().forceEmplace(metadata, std::move(payload));
  } else {
    callback_(metadata, { payload.data(), payload.size() });
  }
}

}  // namespace heph::ipc::zenoh
