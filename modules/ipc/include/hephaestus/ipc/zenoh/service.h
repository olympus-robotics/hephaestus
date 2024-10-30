//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <absl/log/log.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <zenoh.h>
#include <zenoh.hxx>
#include <zenoh/api/encoding.hxx>
#include <zenoh/api/ext/serialization.hxx>
#include <zenoh/api/queryable.hxx>
#include <zenoh/api/session.hxx>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/utils.h"

namespace heph::ipc::zenoh {

template <typename RequestT, typename ReplyT>
class Service {
public:
  using Callback = std::function<ReplyT(const RequestT&)>;
  using FailureCallback = std::function<void()>;
  Service(
      SessionPtr session, TopicConfig topic_config, Callback&& callback,
      FailureCallback&& failure_callback = []() {});

private:
  void onQuery(const ::zenoh::Query& query);

private:
  SessionPtr session_;
  std::unique_ptr<::zenoh::Queryable<void>> queryable_;

  TopicConfig topic_config_;
  Callback callback_;
  FailureCallback failure_callback_;
};

template <typename ReplyT>
struct ServiceResponse {
  std::string topic;
  ReplyT value;
};

template <typename RequestT, typename ReplyT>
auto callService(Session& session, const TopicConfig& topic_config, const RequestT& request,
                 const std::optional<std::chrono::milliseconds>& timeout = std::nullopt)
    -> std::vector<ServiceResponse<ReplyT>>;

// --------- Implementation ----------

namespace internal {
static constexpr auto SERVICE_ATTACHMENT_REQUEST_TYPE_INFO = "0";
static constexpr auto SERVICE_ATTACHMENT_REPLY_TYPE_INFO = "1";

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
    LOG(WARNING) << fmt::format("[Service {}] Query is missing attachments",
                                query.get_keyexpr().as_string_view());
    return false;
  }

  auto attachment_data =
      ::zenoh::ext::deserialize<std::unordered_map<std::string, std::string>>(attachment->get());

  const auto request_type_info = attachment_data[SERVICE_ATTACHMENT_REQUEST_TYPE_INFO];
  const auto reply_type_info = attachment_data[SERVICE_ATTACHMENT_REPLY_TYPE_INFO];

  return request_type_info == utils::getTypeName<RequestT>() &&
         reply_type_info == utils::getTypeName<ReplyT>();
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

    DLOG(INFO) << fmt::format("Deserializing buffer of size: {}", buffer.size());
    RequestT request;
    serdes::deserialize<RequestT>(buffer, request);
    return request;
  }
}

template <class ReplyT>
auto onReply(const ::zenoh::Sample& sample) -> ServiceResponse<ReplyT> {
  const auto server_topic = static_cast<std::string>(sample.get_keyexpr().as_string_view());
  DLOG(INFO) << fmt::format("Received answer of '{}'", server_topic);

  if constexpr (std::is_same_v<ReplyT, std::string>) {
    throwExceptionIf<InvalidParameterException>(
        sample.get_encoding() != ::zenoh::Encoding::Predefined::zenoh_string(),
        fmt::format("Encoding for Service {} should be '{}'", server_topic,
                    ::zenoh::Encoding::Predefined::zenoh_string().as_string()));
    auto payload = sample.get_payload().as_string();
    DLOG(INFO) << fmt::format("Serivce {}: payload is string: '{}'", server_topic, payload);
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
[[nodiscard]] auto createZenohGetOptions(const RequestT& request,
                                         const std::optional<std::chrono::milliseconds>& timeout)
    -> ::zenoh::Session::GetOptions {
  ::zenoh::Session::GetOptions options{};
  if (timeout.has_value()) {
    options.timeout_ms = static_cast<uint64_t>(timeout->count());
  }

  if constexpr (std::is_same_v<RequestT, std::string>) {
    options.encoding = ::zenoh::Encoding::Predefined::zenoh_string();
    options.payload = ::zenoh::ext::serialize(request);
  } else {
    options.encoding = ::zenoh::Encoding::Predefined::zenoh_bytes();
    options.payload = toZenohBytes(serdes::serialize(request));
  }

  std::unordered_map<std::string, std::string> attachments;
  attachments[SERVICE_ATTACHMENT_REQUEST_TYPE_INFO] = utils::getTypeName<RequestT>();
  attachments[SERVICE_ATTACHMENT_REPLY_TYPE_INFO] = utils::getTypeName<ReplyT>();
  options.attachment = ::zenoh::ext::serialize(attachments);

  return options;
}

}  // namespace internal

template <typename RequestT, typename ReplyT>
Service<RequestT, ReplyT>::Service(SessionPtr session, TopicConfig topic_config, Callback&& callback,
                                   FailureCallback&& failure_callback)
  : session_(std::move(session))
  , topic_config_(std::move(topic_config))
  , callback_(std::move(callback))
  , failure_callback_(std::move(failure_callback)) {
  internal::checkTemplatedTypes<RequestT, ReplyT>();
  LOG(INFO) << fmt::format("[Service {}] Started service", topic_config_.name);

  auto on_query_cb = [this](const ::zenoh::Query& query) mutable { onQuery(query); };

  ::zenoh::ZResult result{};
  const ::zenoh::KeyExpr keyexpr{ topic_config_.name };
  queryable_ = std::make_unique<::zenoh::Queryable<void>>(session_->zenoh_session.declare_queryable(
      keyexpr, std::move(on_query_cb), []() {}, ::zenoh::Session::QueryableOptions::create_default(),
      &result));
  throwExceptionIf<FailedZenohOperation>(
      result != Z_OK,
      fmt::format("[Service {}] failed to create zenoh queryable, err {}", topic_config_.name, result));
}

template <typename RequestT, typename ReplyT>
void Service<RequestT, ReplyT>::onQuery(const ::zenoh::Query& query) {
  LOG(INFO) << fmt::format("[Service {}] received query from '{}'", topic_config_.name,
                           query.get_keyexpr().as_string_view());

  ::zenoh::ZResult result{};

  if (!internal::checkQueryTypeInfo<RequestT, ReplyT>(query)) {
    LOG(ERROR) << fmt::format("[Service {}] failed to process query: type mismatch for request and reply",
                              query.get_keyexpr().as_string_view());
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

  LOG_IF(ERROR, result != Z_OK) << fmt::format("[Service {}] failed to reply to query, err {}",
                                               topic_config_.name, result);
}

template <typename RequestT, typename ReplyT>
auto callService(Session& session, const TopicConfig& topic_config, const RequestT& request,
                 const std::optional<std::chrono::milliseconds>& timeout /*= std::nullopt*/)
    -> std::vector<ServiceResponse<ReplyT>> {
  internal::checkTemplatedTypes<RequestT, ReplyT>();

  LOG(INFO) << fmt::format("Calling service on '{}'", topic_config.name);

  const ::zenoh::KeyExpr keyexpr(topic_config.name);
  auto options = internal::createZenohGetOptions<RequestT, ReplyT>(request, timeout);

  ::zenoh::ZResult result{};
  static constexpr auto FIFO_QUEUE_SIZE = 100;
  auto replies = session.zenoh_session.get(keyexpr, "", ::zenoh::channels::FifoChannel(FIFO_QUEUE_SIZE),
                                           std::move(options), &result);
  throwExceptionIf<FailedZenohOperation>(result != Z_OK,
                                         fmt::format("Failed to call service on: {}", topic_config.name));

  std::vector<ServiceResponse<ReplyT>> reply_messages;
  for (auto res = replies.recv(); std::holds_alternative<::zenoh::Reply>(res); res = replies.recv()) {
    const auto& reply = std::get<::zenoh::Reply>(res);
    if (!reply.is_ok()) {
      LOG(ERROR) << fmt::format("Failed to call service on '{}': {}", topic_config.name,
                                reply.get_err().get_payload().as_string());
      continue;
    }

    auto message = internal::onReply<ReplyT>(reply.get_ok());
    reply_messages.emplace_back(std::move(message));
  }

  return reply_messages;
}

}  // namespace heph::ipc::zenoh
