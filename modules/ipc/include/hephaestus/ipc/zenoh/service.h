//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <condition_variable>

#include <absl/log/log.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::ipc::zenoh {

struct StringQueryRequest {
  std::string topic;
  std::string parameters;
  std::string value;
};

class StringService {
public:
  using Callback = std::function<std::string(const StringQueryRequest&)>;
  StringService(SessionPtr session, std::string topic, Callback&& callback);

private:
  SessionPtr session_;
  std::unique_ptr<zenohc::Queryable> queryable_;

  std::string topic_;
  Callback callback_;
};

template <typename TRequest, typename TReply>
class BinaryService {
public:
  using Callback = std::function<TReply(const TRequest&)>;
  explicit BinaryService(SessionPtr session, std::string topic, Callback&& callback);

private:
  SessionPtr session_;
  std::unique_ptr<zenohc::Queryable> queryable_;

  std::string topic_;
  Callback callback_;
};

template <typename TRequest, typename TReply>
auto callBinaryService(const Session& session, const std::string& topic, const TRequest& request,
                       const std::chrono::milliseconds& timeout) -> TReply;

// --------- Implementation ----------

template <typename TRequest, typename TReply>
BinaryService<TRequest, TReply>::BinaryService(SessionPtr session, std::string topic, Callback&& callback)
  : session_(std::move(session)), topic_(std::move(topic)), callback_(std::move(callback)) {
  static_assert(serdes::ProtobufSerializable<TRequest>, "Request needs to be serializable.");
  static_assert(serdes::ProtobufSerializable<TReply>, "Reply needs to be serializable.");

  auto query = [this](const zenohc::Query& query) mutable {
    TRequest request;
    request.Deserialize(query.get_value().as_string_view());
    DLOG(INFO) << fmt::format("Received query from '{}'", query.get_keyexpr().as_string_view());

    TReply reply = this_callback(request);
    zenohc::QueryReplyOptions options;
    options.set_encoding(zenohc::Encoding{ Z_ENCODING_PREFIX_TEXT_PLAIN });
    query.reply(this->topic_, reply.Serialize(), options);
  };

  queryable_ =
      expectAsUniquePtr(session_->zenoh_session.declare_queryable(topic_, { std::move(query), []() {} }));
}

template <typename TRequest, typename TReply>
auto callBinaryService(const SessionPtr& session, const std::string& topic, const TRequest& request,
                       const std::chrono::milliseconds& timeout) -> std::optional<TReply> {
  static_assert(serdes::ProtobufSerializable<TRequest>, "Request needs to be serializable.");
  static_assert(serdes::ProtobufSerializable<TReply>, "Reply needs to be serializable.");

  DLOG(INFO) << fmt::format("Calling service on '{}'", topic);
  std::mutex m;
  std::condition_variable done_signal;
  bool done = false;
  TReply reply_message;

  auto on_reply = [&reply_message](const zenohc::Reply& reply) {
    auto result = reply.get();
    if (const auto& sample = std::get_if<zenohc::Sample>(&result)) {
      DLOG(INFO) << fmt::format("Received answer of '{}'", sample->get_keyexpr().as_string_view());
      reply_message.Deserialize(sample->get_payload().as_string_view());
    } else if (const auto& error = std::get_if<zenohc::ErrorMessage>(&result)) {
      LOG(ERROR) << fmt::format("Received an error on '{}': {}", sample->get_keyexpr().as_string_view(),
                                error->as_string_view());
    }
  };

  auto on_done = [&m, &done, &done_signal]() {
    std::lock_guard lock(m);
    done = true;
    done_signal.notify_all();
  };

  zenohc::ErrNo error_code{};
  zenohc::GetOptions options{};
  options.timeout_ms = static_cast<uint64_t>(timeout.count());
  const auto request_string = serdes::serialize(request);
  options.value = zenohc::Value(request_string, Z_ENCODING_PREFIX_TEXT_PLAIN);
  const auto success =
      session->zenoh_session.get(topic.c_str(), "", { on_reply, on_done }, options, error_code);

  std::unique_lock lock(m);
  if (!done_signal.wait_for(lock, timeout, [&done] { return done; })) {
    LOG(WARNING) << fmt::format("Timeout while waiting for service reply of '{}'", topic);
    return {};
  }

  if (!success) {
    LOG(ERROR) << fmt::format("Error while calling service on '{}': {}", topic, error_code);
    return {};
  }

  return reply_message;
}

}  // namespace heph::ipc::zenoh
