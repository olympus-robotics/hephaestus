//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/publisher.h"

#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/base/exception.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

Publisher::Publisher(SessionPtr session, Config config, MatchCallback&& match_cb)
  : config_(std::move(config)), session_(std::move(session)), match_cb_(std ::move(match_cb)) {
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
  if (config_.real_time) {
    pub_options.set_priority(zenohc::Priority::Z_PRIORITY_REAL_TIME);
  }

  publisher_ = expectAsUniquePtr(session_->declare_publisher(config_.topic, pub_options));

  put_options_.set_encoding(Z_ENCODING_PREFIX_APP_CUSTOM);
  attachment_[messageCounterKey()] = "0";
  attachment_[sessionIdKey()] = toString(session_->info_zid());
  put_options_.set_attachment(attachment_);

  if (match_cb_ != nullptr) {
    using ClosureMatchingListener =
        zenohc::ClosureConstRefParam<zcu_owned_closure_matching_status_t, zcu_matching_status_t,
                                     zcu_matching_status_t>;
    ClosureMatchingListener cb = [this](zcu_matching_status_t matching_status) {
      MatchingStatus status{ .matching = matching_status.matching };
      this->match_cb_(status);
    };

    auto callback = cb.take();
    subscriers_listener_ = zcu_publisher_matching_listener_callback(publisher_->loan(), z_move(callback));
  }
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
