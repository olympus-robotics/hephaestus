//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/raw_publisher.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <zenoh.h>
#include <zenoh/api/base.hxx>
#include <zenoh/api/encoding.hxx>
#include <zenoh/api/enums.hxx>
#include <zenoh/api/ext/advanced_publisher.hxx>
#include <zenoh/api/ext/serialization.hxx>
#include <zenoh/api/ext/session_ext.hxx>
#include <zenoh/api/interop.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/api/liveliness.hxx>
#include <zenoh/api/matching.hxx>
#include <zenoh/api/queryable.hxx>
#include <zenoh/detail/closures.hxx>
#include <zenoh/detail/closures_concrete.hxx>
#include <zenoh_macros.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc::zenoh {
RawPublisher::RawPublisher(SessionPtr session, TopicConfig topic_config, serdes::TypeInfo type_info,
                           MatchCallback&& match_cb)
  : session_(std::move(session))
  , session_ext_(session->zenoh_session)
  , topic_config_(std::move(topic_config))
  , type_info_(std::move(type_info))
  , match_cb_(std ::move(match_cb)) {
  createTypeInfoService();

  auto pub_options = ::zenoh::ext::SessionExt::AdvancedPublisherOptions::create_default();
  if (session_->config.real_time) {
    pub_options.publisher_options.priority = ::zenoh::Priority::Z_PRIORITY_REAL_TIME;
  }

  if (session_->config.cache_size > 0) {
    pub_options.cache->max_samples = session_->config.cache_size;
  }

  pub_options.publisher_detection = true;

  const ::zenoh::KeyExpr keyexpr{ topic_config_.name };
  publisher_ = std::make_unique<::zenoh::ext::AdvancedPublisher>(
      session_ext_.declare_advanced_publisher(keyexpr, std::move(pub_options)));

  if (match_cb_ != nullptr) {
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
  auto type_service_topic = TopicConfig{ .name = getTypeInfoServiceTopic(topic_config_.name) };
  type_service_ = std::make_unique<Service<std::string, std::string>>(session_, type_service_topic,
                                                                      std::move(type_info_callback));
}

void RawPublisher::initializeAttachment() {
  attachment_[PUBLISHER_ATTACHMENT_MESSAGE_SESSION_ID_KEY] = toString(session_->zenoh_session.get_zid());
  attachment_[PUBLISHER_ATTACHMENT_MESSAGE_TYPE_INFO] = type_info_.name;
}
}  // namespace heph::ipc::zenoh
