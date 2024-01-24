//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/publisher.h"

#include "eolo/base/exception.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {
Publisher::Publisher(SessionPtr session, Config config)
  : config_(std::move(config)), session_(std::move(session)) {
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
  publisher_ = expectAsUniquePtr(session_->declare_publisher(config_.topic, pub_options));

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
