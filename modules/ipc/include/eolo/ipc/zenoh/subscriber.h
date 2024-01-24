//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/ipc/common.h"

namespace eolo::ipc::zenoh {

class Subscriber {
public:
  using DataCallback = std::function<void(const MessageMetadata&, std::span<const std::byte>)>;

  Subscriber(Config config, DataCallback&& callback);
  ~Subscriber();
  Subscriber(const Subscriber&) = delete;
  Subscriber(Subscriber&&) = default;
  auto operator=(const Subscriber&) -> Subscriber& = delete;
  auto operator=(Subscriber&&) -> Subscriber& = default;

private:
  void callback(const zenohc::Sample& sample);

private:
  Config config_;
  DataCallback callback_;

  std::unique_ptr<zenohc::Session> session_;
  std::unique_ptr<zenohc::Subscriber> subscriber_;
  ze_owned_querying_subscriber_t cache_subscriber_{};
};

}  // namespace eolo::ipc::zenoh
