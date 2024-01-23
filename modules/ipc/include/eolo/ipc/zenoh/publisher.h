//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>
#include <zenohc.hxx>

#include <zenoh.h>

#include "eolo/base/exception.h"
#include "eolo/ipc/zenoh/config.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

// TODO: Zenoh supports getting list of subscriber https://github.com/eclipse-zenoh/zenoh/pull/565, but it
// appears to not be supported by the C interface

class Publisher {
public:
  explicit Publisher(Config config);
  ~Publisher();
  Publisher(const Publisher&) = delete;
  Publisher(Publisher&&) = default;
  auto operator=(const Publisher&) -> Publisher& = delete;
  auto operator=(Publisher&&) -> Publisher& = default;

  [[nodiscard]] auto publish(std::span<std::byte> data) -> bool;

private:
  Config config_;

  std::unique_ptr<zenohc::Session> session_;
  std::unique_ptr<zenohc::Publisher> publisher_;

  zc_owned_liveliness_token_t liveliness_token_{};
  ze_owned_publication_cache_t pub_cache_{};

  zenohc::PublisherPutOptions put_options_;
  std::size_t pub_msg_count_ = 0;
  std::unordered_map<std::string, std::string> attachment_;
};

Publisher::Publisher(Config config) : config_(std::move(config)) {
  auto zconfig = createZenohConfig(config_);

  // Create the publisher.
  session_ = expectAsPtr(open(std::move(zconfig)));

  // Enable publishing of a liveliness token.
  liveliness_token_ =
      zc_liveliness_declare_token(session_->loan(), z_keyexpr(config_.topic.c_str()), nullptr);
  throwExceptionIf<FailedZenohOperation>(!z_check(liveliness_token_), "failed to create livelines token");

  // Enable cache.
  if (config_.cache_size > 0) {
    auto pub_cache_opts = ze_publication_cache_options_default();
    pub_cache_opts.history = config_.cache_size;
    pub_cache_ =
        ze_declare_publication_cache(session_->loan(), z_keyexpr(config_.topic.c_str()), &pub_cache_opts);
    throwExceptionIf<FailedZenohOperation>(!z_check(pub_cache_), "failed to enable cache");
  }

  zenohc::PublisherOptions pub_options;
  // TODO: add support to priorty
  publisher_ = expectAsPtr(session_->declare_publisher(config_.topic, pub_options));

  put_options_.set_encoding(Z_ENCODING_PREFIX_APP_CUSTOM);
  attachment_[messageCounterKey()] = "0";
  attachment_[sessionIdKey()] = toString(session_->info_zid());
  put_options_.set_attachment(attachment_);
}

Publisher::~Publisher() {
  z_drop(z_move(liveliness_token_));
  z_drop(z_move(pub_cache_));
}

auto Publisher::publish(std::span<std::byte> data) -> bool {
  attachment_[messageCounterKey()] = std::to_string(pub_msg_count_++);

  return publisher_->put({ data.data(), data.size() }, put_options_);
}

}  // namespace eolo::ipc::zenoh
