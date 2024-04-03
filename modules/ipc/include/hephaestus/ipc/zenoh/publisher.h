//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc::zenoh {

struct MatchingStatus {
  bool matching{};  //! If true publisher is connect to at least one subscriber.
};

/// - Create a Zenoh publisher on the topic specified in `config`.
/// - Create a service that provides the schema used to serialize the data.
///   - the service is published on the topic created via `getTypeInfoServiceTopic(topic)`
///     - e.g. for topic `hephaestus/pose` it create a service on `type_info/hephaestus/pose`
///   - the service returns the Json representation of the type info, that can be converted using
///     serdes::TypeInfo::fromJson(str);
/// - If `match_cb` is passed, it is called when the first subscriber matches and when the last one unmatch.
class Publisher {
public:
  using MatchCallback = std::function<void(MatchingStatus)>;
  ///
  Publisher(SessionPtr session, TopicConfig topic_config, serdes::TypeInfo type_info,
            MatchCallback&& match_cb = nullptr);
  ~Publisher();
  Publisher(const Publisher&) = delete;
  Publisher(Publisher&&) = default;
  auto operator=(const Publisher&) -> Publisher& = delete;
  auto operator=(Publisher&&) -> Publisher& = default;

  [[nodiscard]] auto publish(std::span<std::byte> data) -> bool;

  [[nodiscard]] auto id() const -> std::string {
    return toString(session_->zenoh_session.info_zid());
  }

private:
  void enableCache();
  void initAttachments();
  void enableMatchingListener();
  void createTypeInfoService();

private:
  SessionPtr session_;
  TopicConfig topic_config_;
  std::unique_ptr<zenohc::Publisher> publisher_;

  serdes::TypeInfo type_info_;
  std::unique_ptr<StringService> type_service_;

  zc_owned_liveliness_token_t liveliness_token_{};
  ze_owned_publication_cache_t pub_cache_{};

  zenohc::PublisherPutOptions put_options_;
  std::size_t pub_msg_count_ = 0;
  std::unordered_map<std::string, std::string> attachment_;

  MatchCallback match_cb_{ nullptr };
  zcu_owned_matching_listener_t subscriers_listener_{};
};

}  // namespace heph::ipc::zenoh
