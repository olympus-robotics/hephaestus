//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/publisher.h"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include <zenoh.h>
#include <zenoh_macros.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {

RawPublisher::RawPublisher(SessionPtr session, TopicConfig topic_config, serdes::TypeInfo type_info,
                           MatchCallback&& match_cb)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , type_info_(std::move(type_info))
  , match_cb_(std ::move(match_cb)) {
  // Enable publishing of a liveliness token.
  liveliness_token_ = zc_liveliness_declare_token(session_->zenoh_session.loan(),
                                                  z_keyexpr(topic_config_.name.data()), nullptr);
  throwExceptionIf<FailedZenohOperation>(!z_check(liveliness_token_), "failed to create livelines token");

  if (session_->config.cache_size > 0) {
    enableCache();
  }

  zenohc::PublisherOptions pub_options;
  if (session_->config.real_time) {
    pub_options.set_priority(zenohc::Priority::Z_PRIORITY_REAL_TIME);
  }
  publisher_ = expectAsUniquePtr(session_->zenoh_session.declare_publisher(topic_config_.name, pub_options));

  initAttachments();

  if (match_cb_ != nullptr) {
    enableMatchingListener();
  }

  createTypeInfoService();
}

RawPublisher::~RawPublisher() {
  z_drop(z_move(liveliness_token_));
  z_drop(z_move(pub_cache_));
}

auto RawPublisher::publish(std::span<const std::byte> data) -> bool {
  attachment_[messageCounterKey()] = std::to_string(pub_msg_count_++);

  return publisher_->put({ data.data(), data.size() }, put_options_);
}

void RawPublisher::enableCache() {
  auto pub_cache_opts = ze_publication_cache_options_default();
  pub_cache_opts.history = session_->config.cache_size;
  pub_cache_ = ze_declare_publication_cache(session_->zenoh_session.loan(),
                                            z_keyexpr(topic_config_.name.data()), &pub_cache_opts);
  throwExceptionIf<FailedZenohOperation>(!z_check(pub_cache_), "failed to enable cache");
}

void RawPublisher::initAttachments() {
  put_options_.set_encoding(Z_ENCODING_PREFIX_APP_CUSTOM);
  attachment_[messageCounterKey()] = "0";
  attachment_[sessionIdKey()] = toString(session_->zenoh_session.info_zid());
  put_options_.set_attachment(attachment_);

  // TODO: add type info.
}

void RawPublisher::enableMatchingListener() {
  using ClosureMatchingListener = zenohc::ClosureConstRefParam<zcu_owned_closure_matching_status_t,
                                                               zcu_matching_status_t, zcu_matching_status_t>;
  ClosureMatchingListener cb = [this](zcu_matching_status_t matching_status) {
    const MatchingStatus status{ .matching = matching_status.matching };
    this->match_cb_(status);
  };

  auto callback = cb.take();
  subscriers_listener_ = zcu_publisher_matching_listener_callback(publisher_->loan(), z_move(callback));
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
}  // namespace heph::ipc::zenoh
