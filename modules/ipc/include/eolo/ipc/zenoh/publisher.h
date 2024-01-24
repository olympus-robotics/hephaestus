//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

// TODO: Add support to get notified for subscribers: https://github.com/eclipse-zenoh/zenoh-c/pull/236
class Publisher {
public:
  Publisher(SessionPtr session, Config config);
  ~Publisher();
  Publisher(const Publisher&) = delete;
  Publisher(Publisher&&) = default;
  auto operator=(const Publisher&) -> Publisher& = delete;
  auto operator=(Publisher&&) -> Publisher& = default;

  [[nodiscard]] auto publish(std::span<std::byte> data) -> bool;

  [[nodiscard]] auto id() const -> std::string {
    return toString(session_->info_zid());
  }

private:
  Config config_;

  SessionPtr session_;
  std::unique_ptr<zenohc::Publisher> publisher_;

  zc_owned_liveliness_token_t liveliness_token_{};
  ze_owned_publication_cache_t pub_cache_{};

  zenohc::PublisherPutOptions put_options_;
  std::size_t pub_msg_count_ = 0;
  std::unordered_map<std::string, std::string> attachment_;
};

}  // namespace eolo::ipc::zenoh
