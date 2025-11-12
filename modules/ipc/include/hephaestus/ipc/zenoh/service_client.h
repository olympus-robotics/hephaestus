//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "zenoh/api/base.hxx"
#include "zenoh/api/channels.hxx"
#include "zenoh/api/keyexpr.hxx"
#include "zenoh/api/liveliness.hxx"
#include "zenoh/api/querier.hxx"

namespace heph::ipc::zenoh {

template <typename RequestT, typename ReplyT>
class ServiceClient {
public:
  ServiceClient(SessionPtr session, TopicConfig topic_config, std::chrono::milliseconds timeout);
  auto call(const RequestT& request) -> std::vector<ServiceResponse<ReplyT>>;

private:
  SessionPtr session_;
  TopicConfig topic_config_;
  std::unique_ptr<::zenoh::Querier> querier_;
  std::unique_ptr<::zenoh::LivelinessToken> liveliness_token_;
};

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

template <typename RequestT, typename ReplyT>
ServiceClient<RequestT, ReplyT>::ServiceClient(SessionPtr session, TopicConfig topic_config,
                                               std::chrono::milliseconds timeout)
  : session_(std::move(session)), topic_config_(std::move(topic_config)) {
  ::zenoh::Session::QuerierOptions options;
  options.timeout_ms = static_cast<uint64_t>(timeout.count());
  const ::zenoh::KeyExpr keyexpr{ topic_config_.name };

  ::zenoh::ZResult result{};
  querier_ = std::make_unique<::zenoh::Querier>(session_->zenoh_session.declare_querier(
      keyexpr, std::move(options), &result));  // NOLINT(performance-move-const-arg, hicpp-move-const-arg)
  panicIf(result != 0, "[ServiceClient '{}'] failed to create zenoh querier, err {}", topic_config_.name,
          result);

  liveliness_token_ =
      std::make_unique<::zenoh::LivelinessToken>(session_->zenoh_session.liveliness_declare_token(
          generateLivelinessTokenKeyexpr(topic_config_.name, session_->zenoh_session.get_zid(),
                                         EndpointType::SERVICE_CLIENT),
          ::zenoh::Session::LivelinessDeclarationOptions::create_default(), &result));
  panicIf(result != 0, "[ServiceClient {}] failed to create liveliness token, result {}", topic_config_.name,
          result);
}

template <typename RequestT, typename ReplyT>
auto ServiceClient<RequestT, ReplyT>::call(const RequestT& request) -> std::vector<ServiceResponse<ReplyT>> {
  static constexpr auto FIFO_QUEUE_SIZE = 100;

  auto options = internal::createZenohGetOptions<RequestT, ReplyT>(request, {});
  ::zenoh::Querier::GetOptions get_options;
  get_options.payload = std::move(options.payload);
  get_options.encoding = std::move(options.encoding);
  get_options.attachment = std::move(options.attachment);

  ::zenoh::ZResult result{};
  auto replies =
      querier_->get("", ::zenoh::channels::FifoChannel(FIFO_QUEUE_SIZE), std::move(get_options), &result);
  panicIf(result != 0, "Failed to call service on '{}'", topic_config_.name);

  return internal::getServiceCallResponses<ReplyT>(replies);
}

}  // namespace heph::ipc::zenoh
