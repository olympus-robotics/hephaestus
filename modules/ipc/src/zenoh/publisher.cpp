//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/publisher.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include <fmt/core.h>
#include <zenoh.h>
#include <zenoh.hxx>
#include <zenoh/api/keyexpr.hxx>
#include <zenoh/detail/closures_concrete.hxx>
#include <zenoh_macros.h>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/serdes/type_info.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {
namespace {
extern "C" {
inline void zenohOnMatchingStatus(const ::zc_matching_status_t* matching_status, void* context) {
  ::zenoh::detail::closures::IClosure<void, const zc_matching_status_t*>::call_from_context(context,
                                                                                            matching_status);
}
}

template <typename C, typename D>
[[nodiscard]] auto createZenohcClosureMatchingStatus(C&& on_matching_status,
                                                     D&& on_drop) -> zc_owned_closure_matching_status_t {
  zc_owned_closure_matching_status_t c_closure;
  using Cval = std::remove_reference_t<C>;
  using Dval = std::remove_reference_t<D>;
  using ClosureType =
      typename ::zenoh::detail::closures::Closure<Cval, Dval, void, const zc_matching_status_t*>;
  auto closure = ClosureType::into_context(std::forward<C>(on_matching_status), std::forward<D>(on_drop));
  ::z_closure(&c_closure, zenohOnMatchingStatus, ::zenoh::detail::closures::_zenoh_on_drop, closure);
  return c_closure;
}
}  // namespace

RawPublisher::RawPublisher(SessionPtr session, TopicConfig topic_config, serdes::TypeInfo type_info,
                           MatchCallback&& match_cb)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , type_info_(std::move(type_info))
  , enable_cache_(session_->config.cache_size > 0)
  , match_cb_(std ::move(match_cb)) {
  // Enable publishing of a liveliness token.
  ::zenoh::ZResult err{};
  ::zenoh::KeyExpr keyexpr{ topic_config_.name, true, &err };
  throwExceptionIf<FailedZenohOperation>(
      err != Z_OK, fmt::format("[Publisher {}] failed to create keyexpr from topic name, err {}",
                               topic_config_.name, err));
  liveliness_token_ =
      std::make_unique<::zenoh::LivelinessToken>(session_->zenoh_session.liveliness_declare_token(
          keyexpr, ::zenoh::Session::LivelinessDeclarationOptions::create_default(), &err));
  throwExceptionIf<FailedZenohOperation>(
      err != Z_OK,
      fmt::format("[Publisher {}] failed to create livelines token, err {}", topic_config_.name, err));

  if (enable_cache_) {
    enableCache();
  }

  auto pub_options = ::zenoh::Session::PublisherOptions::create_default();
  if (session_->config.real_time) {
    pub_options.priority = ::zenoh::Priority::Z_PRIORITY_REAL_TIME;
  }

  publisher_ = std::make_unique<::zenoh::Publisher>(
      session_->zenoh_session.declare_publisher(keyexpr, std::move(pub_options)));

  if (match_cb_ != nullptr) {
    enableMatchingListener();
  }

  createTypeInfoService();
}

RawPublisher::~RawPublisher() {
  if (enable_cache_) {
    z_drop(z_move(cache_publisher_));
    z_drop(z_move(zenoh_cache_session_));
  }
}

auto RawPublisher::publish(std::span<const std::byte> data) -> bool {
  ::zenoh::ZResult err{};
  auto bytes = toZenohBytes(data);

  auto options = createPublisherOptions();
  publisher_->put(std::move(bytes), std::move(options), &err);
  return err == Z_OK;
}

void RawPublisher::enableCache() {
  ze_publication_cache_options_t cache_publisher_opts;
  ze_publication_cache_options_default(&cache_publisher_opts);
  cache_publisher_opts.history = session_->config.cache_size;

  z_view_keyexpr_t ke;
  z_view_keyexpr_from_str(&ke, topic_config_.name.data());

  zenoh_cache_session_ = std::move(session_->zenoh_session.clone()).take();
  const auto result = ze_declare_publication_cache(&cache_publisher_, z_loan(zenoh_cache_session_),
                                                   z_loan(ke), &cache_publisher_opts);
  throwExceptionIf<FailedZenohOperation>(
      result != Z_OK,
      fmt::format("[Publisher {}] failed to enable cache, err {}", topic_config_.name, result));
}

auto RawPublisher::createPublisherOptions() -> ::zenoh::Publisher::PutOptions {
  auto put_options = ::zenoh::Publisher::PutOptions::create_default();
  put_options.encoding = ::zenoh::Encoding(TEXT_PLAIN_ENCODING);
  attachment_[messageCounterKey()] = std::to_string(pub_msg_count_++);
  attachment_[sessionIdKey()] = toString(session_->zenoh_session.get_zid());
  put_options.attachment = ::zenoh::Bytes::serialize(attachment_);

  return put_options;
}

void RawPublisher::enableMatchingListener() {
  // auto closure = createZenohcClosureMatchingStatus(
  //     [this](const zc_matching_status_t* matching_status) {
  //       const MatchingStatus status{ .matching = matching_status->matching };
  //       this->match_cb_(status);
  //     },
  //     []() {});

  // // TODO: this is a problem because we cannot loan the publisher to the listener.
  // z_loaned_publisher_t pub{};
  // zc_publisher_matching_listener_callback(&subscriers_listener_, &pub, z_move(closure));
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
