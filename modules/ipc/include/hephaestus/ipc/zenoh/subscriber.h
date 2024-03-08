//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"

namespace heph::ipc::zenoh {

class Subscriber {
public:
  using DataCallback = std::function<void(const MessageMetadata&, std::span<const std::byte>)>;

  Subscriber(SessionPtr session, TopicConfig topic_config, DataCallback&& callback);
  ~Subscriber();
  Subscriber(const Subscriber&) = delete;
  Subscriber(Subscriber&&) = default;
  auto operator=(const Subscriber&) -> Subscriber& = delete;
  auto operator=(Subscriber&&) -> Subscriber& = default;

private:
  void callback(const zenohc::Sample& sample);

private:
  SessionPtr session_;
  TopicConfig topic_config_;

  DataCallback callback_;

  std::unique_ptr<zenohc::Subscriber> subscriber_;
  ze_owned_querying_subscriber_t cache_subscriber_{};
};

}  // namespace heph::ipc::zenoh
