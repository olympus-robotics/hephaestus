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
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {

template <typename RequestT, typename ReplyT>
class Service {
public:
  using Callback = std::function<ReplyT(const RequestT&)>;
  Service(SessionPtr session, TopicConfig topic_config, Callback&& callback);

private:
  SessionPtr session_;
  std::unique_ptr<zenohc::Queryable> queryable_;

  TopicConfig topic_config_;
  Callback callback_;
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
// TODO: Remove these functions once zenoh resolves the issue of changing buffers based on encoding.
void addChangingBytes(std::vector<std::byte>& buffer);
auto removeChangingBytes(std::span<const std::byte> buffer) -> std::span<const std::byte>;

template <typename RequestT, typename ReplyT>
constexpr void checkTemplatedTypes() {
  static_assert(serdes::protobuf::ProtobufSerializable<RequestT> || std::is_same_v<RequestT, std::string>,
                "Request needs to be serializable or std::string.");
  static_assert(serdes::protobuf::ProtobufSerializable<ReplyT> || std::is_same_v<ReplyT, std::string>,
                "Reply needs to be serializable or std::string.");
}

template <class RequestT>
auto deserializeRequest(const zenohc::Query& query) -> RequestT {
  if constexpr (std::is_same_v<RequestT, std::string>) {
    throwExceptionIf<InvalidParameterException>(
        query.get_value().get_encoding() != Z_ENCODING_PREFIX_TEXT_PLAIN,
        "Encoding for std::string should be Z_ENCODING_PREFIX_TEXT_PLAIN");
    return static_cast<std::string>(query.get_value().as_string_view());
  } else {
    throwExceptionIf<InvalidParameterException>(
        query.get_value().get_encoding() != Z_ENCODING_PREFIX_EMPTY,
        "Encoding for binary types should be Z_ENCODING_PREFIX_EMPTY");
    auto payload = query.get_value().get_payload();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(payload.start), payload.len);
    buffer = internal::removeChangingBytes(buffer);
    DLOG(INFO) << fmt::format("Deserializing buffer of size: {}", buffer.size());

    RequestT request;
    serdes::deserialize<RequestT>(buffer, request);
    return request;
  }
}

template <class ReplyT>
auto onReply(zenohc::Reply&& reply, std::vector<ServiceResponse<ReplyT>>& reply_messages, std::mutex& m,
             const std::string& topic_name) -> void {
  const auto result = std::move(reply).get();
  if (const auto& error = std::get_if<zenohc::ErrorMessage>(&result)) {
    LOG(ERROR) << fmt::format("Error on service reply for '{}': '{}'", topic_name, error->as_string_view());
    return;
  }

  const auto& sample = std::get_if<zenohc::Sample>(&result);
  const auto server_topic = static_cast<std::string>(sample->get_keyexpr().as_string_view());
  DLOG(INFO) << fmt::format("Received answer of '{}'", server_topic);
  if constexpr (std::is_same_v<ReplyT, std::string>) {
    throwExceptionIf<InvalidParameterException>(
        sample->get_encoding() != Z_ENCODING_PREFIX_TEXT_PLAIN,
        "Encoding for std::string should be Z_ENCODING_PREFIX_TEXT_PLAIN");
    DLOG(INFO) << fmt::format("Payload is string: '{}'", sample->get_payload().as_string_view());
    const std::unique_lock<std::mutex> lock(m);
    reply_messages.emplace_back(ServiceResponse<ReplyT>{
        .topic = server_topic, .value = static_cast<std::string>(sample->get_payload().as_string_view()) });
  } else {
    throwExceptionIf<InvalidParameterException>(
        sample->get_encoding() != Z_ENCODING_PREFIX_EMPTY,
        "Encoding for binary types should be Z_ENCODING_PREFIX_EMPTY");
    auto payload = sample->get_payload();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const std::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(payload.start), payload.len);
    ReplyT reply_deserialized{};
    serdes::deserialize(buffer, reply_deserialized);
    const std::unique_lock<std::mutex> lock(m);
    reply_messages.emplace_back(server_topic, std::move(reply_deserialized));
  }
}
}  // namespace internal

template <typename RequestT, typename ReplyT>
Service<RequestT, ReplyT>::Service(SessionPtr session, TopicConfig topic_config, Callback&& callback)
  : session_(std::move(session)), topic_config_(std::move(topic_config)), callback_(std::move(callback)) {
  internal::checkTemplatedTypes<RequestT, ReplyT>();

  auto query_cb = [this](const zenohc::Query& query) mutable {
    LOG(INFO) << fmt::format("Received query from '{}'", query.get_keyexpr().as_string_view());

    auto reply = this->callback_(internal::deserializeRequest<RequestT>(query));
    zenohc::QueryReplyOptions options;
    if constexpr (std::is_same_v<ReplyT, std::string>) {
      options.set_encoding(zenohc::Encoding{ Z_ENCODING_PREFIX_TEXT_PLAIN });  // Update encoding.
      query.reply(this->topic_config_.name, reply, options);
    } else {
      options.set_encoding(zenohc::Encoding{ Z_ENCODING_PREFIX_EMPTY });  // Update encoding.
      auto buffer = serdes::serialize(reply);
      DLOG(INFO) << fmt::format("Reply: payload size: {}", buffer.size());
      query.reply(this->topic_config_.name, std::move(buffer), options);
    }
  };

  queryable_ = expectAsUniquePtr(
      session_->zenoh_session.declare_queryable(topic_config_.name, { std::move(query_cb), []() {} }));
}

template <typename RequestT, typename ReplyT>
auto callService(Session& session, const TopicConfig& topic_config, const RequestT& request,
                 const std::optional<std::chrono::milliseconds>& timeout /*= std::nullopt*/)
    -> std::vector<ServiceResponse<ReplyT>> {
  internal::checkTemplatedTypes<RequestT, ReplyT>();

  LOG(INFO) << fmt::format("Calling service on '{}'", topic_config.name);
  std::mutex mutex;
  std::condition_variable done_signal;
  std::vector<ServiceResponse<ReplyT>> reply_messages;

  auto on_reply = [&reply_messages, &mutex, &topic_config](zenohc::Reply&& reply) {
    internal::onReply(std::move(reply), reply_messages, mutex, topic_config.name);
  };

  auto on_done = [&mutex, &done_signal]() {
    const std::lock_guard lock(mutex);
    done_signal.notify_all();
  };

  zenohc::GetOptions options{};
  if (timeout.has_value()) {
    options.timeout_ms = static_cast<uint64_t>(timeout->count());
  }

  if constexpr (std::is_same_v<RequestT, std::string>) {
    options.set_value(zenohc::Value(request, Z_ENCODING_PREFIX_TEXT_PLAIN));
  } else {
    auto buffer = serdes::serialize(request);

    DLOG(INFO) << fmt::format("Request: payload size: {}", buffer.size());
    internal::addChangingBytes(buffer);
    auto value = zenohc::Value(std::move(buffer), Z_ENCODING_PREFIX_EMPTY);
    options.set_value(value);
  }

  zenohc::ErrNo error_code{};
  const auto success =
      session.zenoh_session.get(topic_config.name.c_str(), "", { on_reply, on_done }, options, error_code);
  throwExceptionIf<FailedZenohOperation>(!success,
                                         fmt::format("Failed to call service on '{}'", topic_config.name));

  std::unique_lock lock(mutex);
  if (timeout.has_value()) {
    if (done_signal.wait_for(lock, timeout.value()) == std::cv_status::timeout) {
      LOG(WARNING) << fmt::format("Timeout while waiting for service reply of '{}'", topic_config.name);
      return {};
    }
  } else {
    done_signal.wait(lock);
  }

  if (error_code != 0) {
    LOG(ERROR) << fmt::format("Error while calling service on '{}': '{}'", topic_config.name, error_code);
    return {};
  }

  return reply_messages;
}

}  // namespace heph::ipc::zenoh
