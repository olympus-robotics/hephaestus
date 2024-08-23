//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <fmt/core.h>
#include <zenoh/api/liveliness.hxx>

#include "hephaestus/ipc/zenoh/publisher.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::ipc {

template <class T>
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
}  // namespace heph::ipc
