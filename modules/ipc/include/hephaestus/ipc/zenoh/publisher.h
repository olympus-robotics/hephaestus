//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/ipc/zenoh/raw_publisher.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::ipc::zenoh {

template <typename T>
class Publisher {
public:
  Publisher(zenoh::SessionPtr session, TopicConfig topic_config,
            zenoh::RawPublisher::MatchCallback&& match_cb = nullptr)
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
  zenoh::RawPublisher publisher_;
};

template <typename T>
[[nodiscard]] auto
createPublisher(zenoh::SessionPtr session, TopicConfig topic_config,
                zenoh::RawPublisher::MatchCallback&& match_cb = nullptr) -> std::unique_ptr<Publisher<T>> {
  return std::make_unique<Publisher<T>>(std::move(session), std::move(topic_config), std::move(match_cb));
}

}  // namespace heph::ipc::zenoh
