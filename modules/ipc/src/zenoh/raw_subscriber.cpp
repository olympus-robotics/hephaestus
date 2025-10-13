//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/raw_subscriber.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>

#include <absl/strings/numbers.h>
#include <zenoh.h>
#include <zenoh/api/base.hxx>
#include <zenoh/api/ext/advanced_subscriber.hxx>
#include <zenoh/api/ext/serialization.hxx>
#include <zenoh/api/ext/session_ext.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/api/liveliness.hxx>
#include <zenoh/api/sample.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {
namespace {
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

auto RawSubscriber::create(SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
                           serdes::TypeInfo type_info, SubscriberConfig config)
    -> std::shared_ptr<RawSubscriber> {
  auto raw_subscriber = std::shared_ptr<RawSubscriber>(new RawSubscriber{
      std::move(session), std::move(topic_config), std::move(callback), std::move(type_info), config });
  raw_subscriber->setup();
  return raw_subscriber;
}

RawSubscriber::RawSubscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
                             serdes::TypeInfo type_info, SubscriberConfig config)
  : config_(config)
  , session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , type_info_(std::move(type_info))
  , dedicated_callback_thread_(config_.dedicated_callback_thread) {
}

// NOLINTNEXTLINE(bugprone-exception-escape)
RawSubscriber::~RawSubscriber() {
  if (callback_messages_consumer_ != nullptr) {
    auto stopped = callback_messages_consumer_->stop();
    stopped.get();
  }

  try {
    std::move(*subscriber_).undeclare();
    if (liveliness_token_ != nullptr) {
      std::move(*liveliness_token_).undeclare();
    }

  } catch (std::exception& e) {
    heph::log(heph::ERROR, "failed to undeclare subscriber", "topic", topic_config_.name, "exception",
              e.what());
  }
}

void RawSubscriber::setup() {
  if (config_.create_type_info_service) {
    if (type_info_.isValid()) {
      createTypeInfoService();
    } else {
      heph::log(heph::ERROR, "Cannot offer type info service for sub with invalid type info.", "topic",
                topic_config_.name, "type_info", type_info_.toJson());
    }
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

  auto sub_options = ::zenoh::ext::SessionExt::AdvancedSubscriberOptions::create_default();
  if (config_.cache_size.has_value() && *config_.cache_size > 0) {
    sub_options.history =
        ::zenoh::ext::SessionExt::AdvancedSubscriberOptions::HistoryOptions::create_default();
    sub_options.history->max_samples = *config_.cache_size;
  }

  ::zenoh::ZResult result{};
  const ::zenoh::KeyExpr keyexpr{ topic_config_.name };
  subscriber_ = std::make_unique<::zenoh::ext::AdvancedSubscriber<void>>(
      session_->zenoh_session.ext().declare_advanced_subscriber(
          keyexpr, [self = shared_from_this()](const ::zenoh::Sample& sample) { self->callback(sample); },
          []() {}, std::move(sub_options), &result));
  panicIf(result != Z_OK, "[Subscriber {}] failed to create zenoh subscriber, err {}", topic_config_.name,
          result);

  if (config_.create_liveliness_token) {
    liveliness_token_ =
        std::make_unique<::zenoh::LivelinessToken>(session_->zenoh_session.liveliness_declare_token(
            generateLivelinessTokenKeyexpr(topic_config_.name, session_->zenoh_session.get_zid(),
                                           EndpointType::SUBSCRIBER),
            ::zenoh::Session::LivelinessDeclarationOptions::create_default(), &result));
  }
}

void RawSubscriber::callback(const ::zenoh::Sample& sample) {
  const auto metadata = getMetadata(sample, topic_config_.name);
  auto payload = toByteVector(sample.get_payload());

  if (dedicated_callback_thread_) {
    auto dropped_element = callback_messages_consumer_->queue().forceEmplace(metadata, std::move(payload));
    logIf(heph::ERROR, dropped_element.has_value(), "Dropped subscriber message due to full queue", "topic",
          topic_config_.name);
  } else {
    callback_(metadata, { payload.data(), payload.size() });
  }
}

void RawSubscriber::createTypeInfoService() {
  auto type_info_json = this->type_info_.toJson();
  auto type_info_callback = [type_info_json](const auto& request) {
    (void)request;
    return type_info_json;
  };

  auto failure_callback = [topic_name = topic_config_.name]() {
    heph::log(heph::ERROR, "Failed to process type info service", "topic", topic_name);
  };

  auto post_reply_callback = []() {
    // Do nothing.
  };

  const ServiceConfig service_config = {
    .create_liveliness_token = false,
    .create_type_info_service = false,
  };

  type_service_ = std::make_unique<Service<std::string, std::string>>(
      session_, TopicConfig{ getEndpointTypeInfoServiceTopic(topic_config_.name) },
      std::move(type_info_callback), failure_callback, post_reply_callback, service_config);
}

}  // namespace heph::ipc::zenoh
