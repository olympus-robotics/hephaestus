//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include <zenoh.h>
#include <zenoh.hxx>

#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::ipc::zenoh {

template <typename T>
class Subscriber {
public:
  using DataCallback = std::function<void(const MessageMetadata&, const std::shared_ptr<T>&)>;
  Subscriber(zenoh::SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
             bool dedicated_callback_thread = false)
    : subscriber_(
          std::move(session), std::move(topic_config),
          [callback = std::move(callback)](const MessageMetadata& metadata,
                                           std::span<const std::byte> buffer) mutable {
            auto data = std::make_shared<T>();
            heph::serdes::deserialize(buffer, *data);
            callback(metadata, std::move(data));
          },
          dedicated_callback_thread) {
  }

private:
  RawSubscriber subscriber_;
};

template <typename T>
[[nodiscard]] auto createSubscriber(zenoh::SessionPtr session, TopicConfig topic_config,
                                    typename Subscriber<T>::DataCallback&& callback,
                                    bool dedicated_callback_thread = false)
    -> std::unique_ptr<Subscriber<T>> {
  return std::make_unique<Subscriber<T>>(std::move(session), std::move(topic_config), std::move(callback),
                                         dedicated_callback_thread);
}

}  // namespace heph::ipc::zenoh
