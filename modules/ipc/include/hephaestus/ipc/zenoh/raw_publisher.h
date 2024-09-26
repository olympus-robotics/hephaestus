//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include <zenoh.h>
#include <zenoh.hxx>

#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
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
class RawPublisher {
public:
  using MatchCallback = std::function<void(MatchingStatus)>;
  ///
  RawPublisher(SessionPtr session, TopicConfig topic_config, serdes::TypeInfo type_info,
               MatchCallback&& match_cb = nullptr);
  ~RawPublisher();
  RawPublisher(const RawPublisher&) = delete;
  RawPublisher(RawPublisher&&) = delete;
  auto operator=(const RawPublisher&) -> RawPublisher& = delete;
  auto operator=(RawPublisher&&) -> RawPublisher& = delete;

  [[nodiscard]] auto publish(std::span<const std::byte> data) -> bool;

  [[nodiscard]] auto id() const -> std::string {
    return toString(session_->zenoh_session.get_zid());
  }

private:
  void enableCache();
  [[nodiscard]] auto createPublisherOptions() -> ::zenoh::Publisher::PutOptions;
  void enableMatchingListener();
  void createTypeInfoService();

private:
  SessionPtr session_;
  TopicConfig topic_config_;
  std::unique_ptr<::zenoh::Publisher> publisher_;

  serdes::TypeInfo type_info_;
  std::unique_ptr<Service<std::string, std::string>> type_service_;

  std::unique_ptr<::zenoh::LivelinessToken> liveliness_token_;

  bool enable_cache_ = false;
  ze_owned_publication_cache_t cache_publisher_{};

  std::size_t pub_msg_count_ = 0;
  std::unordered_map<std::string, std::string> attachment_;

  MatchCallback match_cb_{ nullptr };
  zc_owned_matching_listener_t subscriers_listener_{};
};

[[nodiscard]] static inline auto getTypeInfoServiceTopic(const std::string& topic) -> std::string {
  return fmt::format("type_info/{}", topic);
}

}  // namespace heph::ipc::zenoh
