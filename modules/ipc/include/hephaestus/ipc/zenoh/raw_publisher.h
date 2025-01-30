//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>

#include <fmt/format.h>
#include <zenoh.h>
#include <zenoh/api/ext/advanced_publisher.hxx>
#include <zenoh/api/ext/session_ext.hxx>
#include <zenoh/api/liveliness.hxx>

#include "hephaestus/ipc/topic.h"
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
  ~RawPublisher() = default;
  RawPublisher(const RawPublisher&) = delete;
  RawPublisher(RawPublisher&&) = delete;
  auto operator=(const RawPublisher&) -> RawPublisher& = delete;
  auto operator=(RawPublisher&&) -> RawPublisher& = delete;

  [[nodiscard]] auto publish(std::span<const std::byte> data) -> bool;

  [[nodiscard]] auto sessionId() const -> std::string {
    return toString(session_->zenoh_session.get_zid());
  }

private:
  [[nodiscard]] auto createPublisherOptions() -> ::zenoh::ext::AdvancedPublisher::PutOptions;
  void createTypeInfoService();
  void initializeAttachment();

private:
  SessionPtr session_;

  TopicConfig topic_config_;
  std::unique_ptr<::zenoh::ext::AdvancedPublisher> publisher_;
  std::unique_ptr<::zenoh::LivelinessToken> liveliness_token_;

  serdes::TypeInfo type_info_;
  std::unique_ptr<Service<std::string, std::string>> type_service_;

  std::size_t pub_msg_count_ = 0;
  std::unordered_map<std::string, std::string> attachment_;

  MatchCallback match_cb_{ nullptr };
  std::unique_ptr<::zenoh::MatchingListener<void>> matching_listener_;
};

[[nodiscard]] static inline auto getTypeInfoServiceTopic(const std::string& topic) -> std::string {
  return fmt::format("type_info/{}", topic);
}

}  // namespace heph::ipc::zenoh
