//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/raw_publisher.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include <zenoh.h>
#include <zenoh/api/base.hxx>
#include <zenoh/api/encoding.hxx>
#include <zenoh/api/ext/advanced_publisher.hxx>
#include <zenoh/api/ext/serialization.hxx>
#include <zenoh/api/ext/session_ext.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/api/liveliness.hxx>
#include <zenoh/api/matching.hxx>
#include <zenoh/api/queryable.hxx>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc::zenoh {
RawPublisher::RawPublisher(SessionPtr session, TopicConfig topic_config, serdes::TypeInfo type_info,
                           MatchCallback&& match_cb, const PublisherConfig& config)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , type_info_(std::move(type_info))
  , match_cb_(std ::move(match_cb)) {
  if (config.create_type_info_service) {
    createTypeInfoService();
  }

  auto pub_options = ::zenoh::ext::SessionExt::AdvancedPublisherOptions::create_default();
  pub_options.publisher_options = config.zenoh_publisher_options;

  if (config.cache_size.has_value()) {
    pub_options.cache = ::zenoh::ext::SessionExt::AdvancedPublisherOptions::CacheOptions::create_default();
    pub_options.cache->max_samples = *config.cache_size;
  }

  const ::zenoh::KeyExpr keyexpr{ topic_config_.name };
  publisher_ = std::make_unique<::zenoh::ext::AdvancedPublisher>(
      session_->zenoh_session.ext().declare_advanced_publisher(keyexpr, std::move(pub_options)));

  if (config.create_liveliness_token) {
    ::zenoh::ZResult result{};
    liveliness_token_ =
        std::make_unique<::zenoh::LivelinessToken>(session_->zenoh_session.liveliness_declare_token(
            generateLivelinessTokenKeyexpr(topic_config_.name, session_->zenoh_session.get_zid(),
                                           EndpointType::PUBLISHER),
            ::zenoh::Session::LivelinessDeclarationOptions::create_default(), &result));
    panicIf(result != Z_OK, "[Publisher {}] failed to create liveliness token, result {}", topic_config_.name,
            result);
  }

  if (match_cb_) {
    matching_listener_ =
        std::make_unique<::zenoh::MatchingListener<void>>(publisher_->declare_matching_listener(
            [this](const ::zenoh::MatchingStatus& matching_status) {
              this->match_cb_({ .matching = matching_status.matching });
            },
            []() {}));
  }

  initializeAttachment();
}

auto RawPublisher::publish(std::span<const std::byte> data) -> bool {
  ::zenoh::ZResult result{};
  auto bytes = toZenohBytes(data);

  auto options = createPublisherOptions();
  publisher_->put(std::move(bytes), std::move(options), &result);
  return result == Z_OK;
}

auto RawPublisher::createPublisherOptions() -> ::zenoh::ext::AdvancedPublisher::PutOptions {
  auto put_options = ::zenoh::Publisher::PutOptions::create_default();
  put_options.encoding = ::zenoh::Encoding::Predefined::zenoh_bytes();
  attachment_[PUBLISHER_ATTACHMENT_MESSAGE_COUNTER_KEY] = std::to_string(pub_msg_count_++);
  put_options.attachment = ::zenoh::ext::serialize(attachment_);

  return { .put_options = std::move(put_options) };
}

void RawPublisher::createTypeInfoService() {
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

void RawPublisher::initializeAttachment() {
  attachment_[PUBLISHER_ATTACHMENT_MESSAGE_SESSION_ID_KEY] = toString(session_->zenoh_session.get_zid());
  attachment_[PUBLISHER_ATTACHMENT_MESSAGE_TYPE_INFO] = type_info_.name;
}
}  // namespace heph::ipc::zenoh
