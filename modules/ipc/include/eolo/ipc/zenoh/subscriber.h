//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <span>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/base/exception.h"
#include "eolo/ipc/zenoh/config.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

struct MessageMetadata {
  // TODO: convert this to a uuid
  std::string sender_id;
  std::chrono::nanoseconds timestamp;
  std::size_t sequence_id;
};

class Subscriber {
public:
  using DataCallback = std::function<void(const MessageMetadata&, std::span<const std::byte>)>;

  Subscriber(Config config, DataCallback&& callback);

private:
  void callback(const zenohc::Sample& sample);

private:
  Config config_;
  DataCallback callback_;

  std::unique_ptr<zenohc::Session> session_;
  std::unique_ptr<zenohc::Subscriber> subscriber_;
  ze_owned_querying_subscriber_t cache_subscriber_{};
};

Subscriber::Subscriber(Config config, DataCallback&& callback)
  : config_(std::move(config)), callback_(std::move(callback)) {
  auto zconfig = createZenohConfig(config_);

  // Create the subscriber.
  session_ = expectAsPtr(open(std::move(zconfig)));

  if (config_.cache_size > 0) {
    auto cb = [this](const zenohc::Sample& sample) { this->callback(sample); };
    subscriber_ = expectAsPtr(session_->declare_subscriber(config_.topic, std::move(cb)));
  } else {
    const auto sub_opts = ze_querying_subscriber_options_default();
    zenohc::ClosureSample cb = [this](const zenohc::Sample& sample) { this->callback(sample); };

    auto c = cb.take();
    cache_subscriber_ = ze_declare_querying_subscriber(session_->loan(), z_keyexpr(config_.topic.c_str()),
                                                       z_move(c), &sub_opts);
    eolo::throwExceptionIf<eolo::FailedZenohOperation>(!z_check(cache_subscriber_),
                                                       "failed to create zenoh sub");
  }
}

void Subscriber::callback(const zenohc::Sample& sample) {
  std::println("Received data: {}", sample.get_keyexpr().as_string_view());

  MessageMetadata metadata;
  if (sample.get_attachment().check()) {
    auto sequence_id = std::string{ sample.get_attachment().get(messageCounterKey()).as_string_view() };
    metadata.sequence_id = std::stoul(sequence_id);
    metadata.sender_id = sample.get_attachment().get(sessionIdKey()).as_string_view();
  }

  metadata.timestamp = toChrono(sample.get_timestamp());

  auto buffer = toByteSpan(sample.get_payload());
  callback_(metadata, buffer);
}

}  // namespace eolo::ipc::zenoh
