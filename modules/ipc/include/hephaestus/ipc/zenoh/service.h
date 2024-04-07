//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <condition_variable>
#include <mutex>

#include <absl/log/log.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/serdes/serdes.h"

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
auto callService(const SessionPtr& session, const TopicConfig& topic_config, const RequestT& request,
                 const std::optional<std::chrono::milliseconds>& timeout = std::nullopt)
    -> std::vector<ServiceResponse<ReplyT>>;

// --------- Implementation ----------

namespace internal {
template <typename RequestT, typename ReplyT>
constexpr void checkTemplatedTypes() {
  static_assert(serdes::ProtobufSerializable<RequestT> || std::is_same_v<RequestT, std::string>,
                "Request needs to be serializable or std::string.");
  static_assert(serdes::ProtobufSerializable<ReplyT> || std::is_same_v<ReplyT, std::string>,
                "Reply needs to be serializable or std::string.");
}

template <class RequestT>
auto createRequest(const zenohc::Query& query) -> RequestT {
  if constexpr (std::is_same_v<RequestT, std::string>) {
    return static_cast<std::string>(query.get_value().as_string_view());
  } else {
    RequestT request;
    auto payload = query.get_value().get_payload();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(payload.start), payload.len);
    serdes::deserialize<RequestT>(buffer, request);
    return request;
  }
}
}  // namespace internal

template <typename RequestT, typename ReplyT>
Service<RequestT, ReplyT>::Service(SessionPtr session, TopicConfig topic_config, Callback&& callback)
  : session_(std::move(session)), topic_config_(std::move(topic_config)), callback_(std::move(callback)) {
  internal::checkTemplatedTypes<RequestT, ReplyT>();

  auto query = [this](const zenohc::Query& query) mutable {
    DLOG(INFO) << fmt::format("Received query from '{}'", query.get_keyexpr().as_string_view());

    auto reply = this->callback_(internal::createRequest<RequestT>(query));
    zenohc::QueryReplyOptions options;
    if constexpr (std::is_same_v<ReplyT, std::string>) {
      options.set_encoding(zenohc::Encoding{ Z_ENCODING_PREFIX_TEXT_PLAIN });  // Update encoding.
      query.reply(this->topic_config_.name, reply, options);
    } else {
      options.set_encoding(zenohc::Encoding{ Z_ENCODING_PREFIX_APP_CUSTOM });  // Update encoding.
      query.reply(this->topic_config_.name, serdes::serialize(reply), options);
    }
  };

  queryable_ = expectAsUniquePtr(
      session_->zenoh_session.declare_queryable(topic_config_.name, { std::move(query), []() {} }));
}

template <typename RequestT, typename ReplyT>
auto callService(const SessionPtr& session, const TopicConfig& topic_config, const RequestT& request,
                 const std::optional<std::chrono::milliseconds>& timeout /*= std::nullopt*/)
    -> std::vector<ServiceResponse<ReplyT>> {
  internal::checkTemplatedTypes<RequestT, ReplyT>();

  DLOG(INFO) << fmt::format("Calling service on '{}'", topic_config.name);
  std::mutex m;
  std::condition_variable done_signal;
  std::vector<ServiceResponse<ReplyT>> reply_messages;

  auto on_reply = [&reply_messages, &m, &topic_config](zenohc::Reply&& reply) {
    const auto result = std::move(reply).get();
    if (const auto& error = std::get_if<zenohc::ErrorMessage>(&result)) {
      LOG(ERROR) << fmt::format("Error on {} reply: {}", topic_config.name, error->as_string_view());
      return;
    }

    const auto& sample = std::get_if<zenohc::Sample>(&result);
    DLOG(INFO) << fmt::format("Received answer of '{}'", sample->get_keyexpr().as_string_view());
    if constexpr (std::is_same_v<ReplyT, std::string>) {
      std::unique_lock<std::mutex> lock(m);
      reply_messages.emplace_back(ServiceResponse<ReplyT>{
          .topic = topic_config.name,
          .value = static_cast<std::string>(sample->get_payload().as_string_view()) });
    } else {
      auto payload = sample->get_payload();
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      std::span<const std::byte> buffer(reinterpret_cast<const std::byte*>(payload.start), payload.len);
      std::unique_lock<std::mutex> lock(m);
      ReplyT reply_deserialized;
      serdes::deserialize(buffer, reply_deserialized);
      reply_messages.emplace_back(
          ServiceResponse<ReplyT>{ .topic = topic_config.name, .value = std::move(reply_deserialized) });
    }
  };

  auto on_done = [&m, &done_signal]() {
    std::lock_guard lock(m);
    done_signal.notify_all();
  };

  zenohc::ErrNo error_code{};
  zenohc::GetOptions options{};
  if (timeout.has_value()) {
    options.timeout_ms = static_cast<uint64_t>(timeout->count());
  }

  if constexpr (std::is_same_v<RequestT, std::string>) {
    options.set_value(zenohc::Value(request, Z_ENCODING_PREFIX_TEXT_PLAIN));
  } else {
    options.set_value(zenohc::Value(serdes::serialize(request), Z_ENCODING_PREFIX_APP_CUSTOM));
  }
  const auto success =
      session->zenoh_session.get(topic_config.name.c_str(), "", { on_reply, on_done }, options, error_code);

  std::unique_lock lock(m);
  if (timeout.has_value()) {
    if (done_signal.wait_for(lock, timeout.value()) == std::cv_status::timeout) {
      LOG(WARNING) << fmt::format("Timeout while waiting for service reply of '{}'", topic_config.name);
      return {};
    }
  } else {
    done_signal.wait(lock);
  }

  if (!success) {
    LOG(ERROR) << fmt::format("Error while calling service on '{}': {}", topic_config.name, error_code);
    return {};
  }

  return reply_messages;
}

}  // namespace heph::ipc::zenoh
