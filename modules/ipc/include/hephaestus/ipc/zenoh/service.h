//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <zenoh.h>
#include <zenoh/api/base.hxx>
#include <zenoh/api/channels.hxx>
#include <zenoh/api/encoding.hxx>
#include <zenoh/api/ext/serialization.hxx>
#include <zenoh/api/query.hxx>
#include <zenoh/api/queryable.hxx>
#include <zenoh/api/reply.hxx>
#include <zenoh/api/sample.hxx>
#include <zenoh/api/session.hxx>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {

template <typename RequestT, typename ReplyT>
class Service {
public:
  using Callback = std::function<ReplyT(const RequestT&)>;
  using FailureCallback = std::function<void()>;
  using PostReplyCallback = std::function<void()>;
  Service(
      SessionPtr session, TopicConfig topic_config, Callback&& callback,
      FailureCallback&& failure_callback = []() {}, PostReplyCallback&& post_reply_callback = []() {});

private:
  void onQuery(const ::zenoh::Query& query);

private:
  SessionPtr session_;
  std::unique_ptr<::zenoh::Queryable<void>> queryable_;

  TopicConfig topic_config_;
  Callback callback_;
  FailureCallback failure_callback_;
  PostReplyCallback post_reply_callback_;
};

template <typename ReplyT>
struct ServiceResponse {
  std::string topic;
  ReplyT value;
};

template <typename RequestT, typename ReplyT>
auto callService(Session& session, const TopicConfig& topic_config, const RequestT& request,
                 std::chrono::milliseconds timeout) -> std::vector<ServiceResponse<ReplyT>>;

// -----------------------------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------------------------

namespace internal {
static constexpr auto SERVICE_ATTACHMENT_REQUEST_TYPE_INFO = "0";
static constexpr auto SERVICE_ATTACHMENT_REPLY_TYPE_INFO = "1";

template <typename T>
[[nodiscard]] auto getSerializedTypeName() -> std::string {
  if constexpr (std::is_same_v<T, std::string>) {
    // NOTE: we manually write the type name to avoid namespace conflicts between GCC and clang.
    return "std::string";
  } else {
    return serdes::getSerializedTypeInfo<T>().name;
  }
}

template <typename RequestT, typename ReplyT>
constexpr void checkTemplatedTypes() {
  static_assert(serdes::protobuf::ProtobufSerializable<RequestT> || std::is_same_v<RequestT, std::string>,
                "Request needs to be serializable or std::string.");
  static_assert(serdes::protobuf::ProtobufSerializable<ReplyT> || std::is_same_v<ReplyT, std::string>,
                "Reply needs to be serializable or std::string.");
}

template <typename RequestT, typename ReplyT>
[[nodiscard]] auto checkQueryTypeInfo(const ::zenoh::Query& query) -> bool {
  const auto attachment = query.get_attachment();
  if (!attachment.has_value()) {
    heph::log(heph::WARN, "query is missing attachments", "service", query.get_keyexpr().as_string_view());
    return false;
  }

  auto attachment_data =
      ::zenoh::ext::deserialize<std::unordered_map<std::string, std::string>>(attachment->get());

  const auto request_type_info = attachment_data[SERVICE_ATTACHMENT_REQUEST_TYPE_INFO];
  const auto reply_type_info = attachment_data[SERVICE_ATTACHMENT_REPLY_TYPE_INFO];

  const auto this_request_type = getSerializedTypeName<RequestT>();
  const auto this_reply_type = getSerializedTypeName<ReplyT>();

  return request_type_info == this_request_type && reply_type_info == this_reply_type;
}

template <class RequestT>
auto deserializeRequest(const ::zenoh::Query& query) -> RequestT {
  const auto& keyexpr = query.get_keyexpr().as_string_view();

  const auto encoding = query.get_encoding();
  throwExceptionIf<InvalidParameterException>(
      !encoding.has_value(), fmt::format("Serivce {}: encoding is missing in query.", keyexpr));

  auto payload = query.get_payload();
  throwExceptionIf<InvalidParameterException>(
      !payload.has_value(), fmt::format("Serivce {}: payload is missing in query.", keyexpr));

  if constexpr (std::is_same_v<RequestT, std::string>) {
    throwExceptionIf<InvalidParameterException>(
        encoding.value().get() !=  // NOLINT(bugprone-unchecked-optional-access)
            ::zenoh::Encoding::Predefined::zenoh_string(),
        fmt::format("Encoding for std::string should be '{}'",
                    ::zenoh::Encoding::Predefined::zenoh_string().as_string()));

    return payload->get().as_string();  // NOLINT(bugprone-unchecked-optional-access)
  } else {
    throwExceptionIf<InvalidParameterException>(
        encoding.value().get() !=  // NOLINT(bugprone-unchecked-optional-access)
            ::zenoh::Encoding::Predefined::zenoh_bytes(),
        fmt::format("Encoding for std::string should be '{}'",
                    ::zenoh::Encoding::Predefined::zenoh_bytes().as_string()));

    auto buffer = toByteVector(payload->get());  // NOLINT(bugprone-unchecked-optional-access)

    RequestT request;
    serdes::deserialize<RequestT>(buffer, request);
    return request;
  }
}

template <class ReplyT>
auto onReply(const ::zenoh::Sample& sample) -> ServiceResponse<ReplyT> {
  const auto server_topic = static_cast<std::string>(sample.get_keyexpr().as_string_view());

  if constexpr (std::is_same_v<ReplyT, std::string>) {
    throwExceptionIf<InvalidParameterException>(
        sample.get_encoding() != ::zenoh::Encoding::Predefined::zenoh_string(),
        fmt::format("Encoding for Service {} should be '{}'", server_topic,
                    ::zenoh::Encoding::Predefined::zenoh_string().as_string()));
    auto payload = sample.get_payload().as_string();
    return ServiceResponse<ReplyT>{ .topic = server_topic, .value = std::move(payload) };
  } else {
    throwExceptionIf<InvalidParameterException>(
        sample.get_encoding() != ::zenoh::Encoding::Predefined::zenoh_bytes(),
        fmt::format("Encoding for Service {} should be '{}'", server_topic,
                    ::zenoh::Encoding::Predefined::zenoh_bytes().as_string()));
    auto buffer = toByteVector(sample.get_payload());
    ReplyT reply{};
    serdes::deserialize(buffer, reply);
    return ServiceResponse<ReplyT>{ .topic = server_topic, .value = std::move(reply) };
  }
}

template <typename RequestT, typename ReplyT>
[[nodiscard]] auto createZenohGetOptions(const RequestT& request, std::chrono::milliseconds timeout)
    -> ::zenoh::Session::GetOptions {
  ::zenoh::Session::GetOptions options{};
  options.timeout_ms = static_cast<uint64_t>(timeout.count());

  if constexpr (std::is_same_v<RequestT, std::string>) {
    options.encoding = ::zenoh::Encoding::Predefined::zenoh_string();
    options.payload = ::zenoh::ext::serialize(request);
  } else {
    options.encoding = ::zenoh::Encoding::Predefined::zenoh_bytes();
    options.payload = toZenohBytes(serdes::serialize(request));
  }

  std::unordered_map<std::string, std::string> attachments;
  attachments[SERVICE_ATTACHMENT_REQUEST_TYPE_INFO] = getSerializedTypeName<RequestT>();
  attachments[SERVICE_ATTACHMENT_REPLY_TYPE_INFO] = getSerializedTypeName<ReplyT>();
  options.attachment = ::zenoh::ext::serialize(attachments);

  return options;
}

}  // namespace internal

template <typename RequestT, typename ReplyT>
Service<RequestT, ReplyT>::Service(SessionPtr session, TopicConfig topic_config, Callback&& callback,
                                   FailureCallback&& failure_callback,
                                   PostReplyCallback&& post_reply_callback)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , failure_callback_(std::move(failure_callback))
  , post_reply_callback_(std::move(post_reply_callback)) {
  internal::checkTemplatedTypes<RequestT, ReplyT>();
  heph::log(heph::DEBUG, "started service", "name", topic_config_.name);

  auto on_query_cb = [this](const ::zenoh::Query& query) mutable { onQuery(query); };

  ::zenoh::ZResult result{};
  const ::zenoh::KeyExpr keyexpr{ topic_config_.name };
  queryable_ = std::make_unique<::zenoh::Queryable<void>>(session_->zenoh_session.declare_queryable(
      keyexpr, std::move(on_query_cb), []() {}, ::zenoh::Session::QueryableOptions::create_default(),
      &result));
  throwExceptionIf<FailedZenohOperation>(
      result != Z_OK,
      fmt::format("[Service '{}'] failed to create zenoh queryable, err {}", topic_config_.name, result));
}

template <typename RequestT, typename ReplyT>
void Service<RequestT, ReplyT>::onQuery(const ::zenoh::Query& query) {
  heph::log(heph::DEBUG, "received query", "service", topic_config_.name, "from",
            query.get_keyexpr().as_string_view());

  ::zenoh::ZResult result{};

  if (!internal::checkQueryTypeInfo<RequestT, ReplyT>(query)) {
    heph::log(heph::ERROR, "failed to process query", "error", "type mismatch for request and reply",
              "service", query.get_keyexpr().as_string_view());
    failure_callback_();
    query.reply_err(::zenoh::ext::serialize("Type mismatch for request and reply"),
                    ::zenoh::Query::ReplyErrOptions::create_default(), &result);
    return;
  }

  auto reply = this->callback_(internal::deserializeRequest<RequestT>(query));
  ::zenoh::Query::ReplyOptions options;
  if constexpr (std::is_same_v<ReplyT, std::string>) {
    options.encoding = ::zenoh::Encoding::Predefined::zenoh_string();
    query.reply(this->topic_config_.name, reply, std::move(options), &result);
  } else {
    options.encoding = ::zenoh::Encoding::Predefined::zenoh_bytes();
    auto buffer = serdes::serialize(reply);
    query.reply(this->topic_config_.name, toZenohBytes(buffer), std::move(options), &result);
  }

  heph::logIf(heph::ERROR, result != Z_OK, "failed to reply to query", "service", topic_config_.name, "error",
              result);

  post_reply_callback_();
}

template <typename RequestT, typename ReplyT>
auto callService(Session& session, const TopicConfig& topic_config, const RequestT& request,
                 std::chrono::milliseconds timeout) -> std::vector<ServiceResponse<ReplyT>> {
  internal::checkTemplatedTypes<RequestT, ReplyT>();

  heph::log(heph::DEBUG, "calling service", "topic", topic_config.name);

  const ::zenoh::KeyExpr keyexpr(topic_config.name);
  auto options = internal::createZenohGetOptions<RequestT, ReplyT>(request, timeout);

  ::zenoh::ZResult result{};
  static constexpr auto FIFO_QUEUE_SIZE = 100;
  auto replies = session.zenoh_session.get(keyexpr, "", ::zenoh::channels::FifoChannel(FIFO_QUEUE_SIZE),
                                           std::move(options), &result);
  throwExceptionIf<FailedZenohOperation>(result != Z_OK,
                                         fmt::format("Failed to call service on '{}'", topic_config.name));

  std::vector<ServiceResponse<ReplyT>> reply_messages;
  for (auto res = replies.recv(); std::holds_alternative<::zenoh::Reply>(res); res = replies.recv()) {
    const auto& reply = std::get<::zenoh::Reply>(res);
    if (!reply.is_ok()) {
      heph::log(heph::ERROR, "failed to call service", "topic", topic_config.name, "error",
                reply.get_err().get_payload().as_string());
      continue;
    }

    auto message = internal::onReply<ReplyT>(reply.get_ok());
    reply_messages.emplace_back(std::move(message));
  }

  return reply_messages;
}

}  // namespace heph::ipc::zenoh
