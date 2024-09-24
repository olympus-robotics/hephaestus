//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <string>
#include <utility>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/raw_publisher.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::ipc::zenoh {

template <class T>
class Publisher {
public:
  Publisher(SessionPtr session, TopicConfig topic_config, RawPublisher::MatchCallback&& match_cb = nullptr)
    : publisher_(std::move(session), std::move(topic_config), serdes::getSerializedTypeInfo<T>(),
                 std::move(match_cb)) {
  }

  [[nodiscard]] auto publish(const T& data) -> bool {
    auto buffer = serdes::serialize(data);
    return publisher_.publish(buffer);
  }

  [[nodiscard]] auto id() const -> std::string {
    return publisher_.id();
  }

private:
  RawPublisher publisher_;
};

}  // namespace heph::ipc::zenoh
