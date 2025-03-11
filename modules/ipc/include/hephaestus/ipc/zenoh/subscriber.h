//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/raw_subscriber.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/utils/exception.h"

namespace heph::ipc::zenoh {

class SubscriberBase {
public:
  virtual ~SubscriberBase() = default;
};

template <typename T>
class Subscriber : public SubscriberBase {
public:
  using DataCallback = std::function<void(const MessageMetadata&, const std::shared_ptr<T>&)>;
  Subscriber(zenoh::SessionPtr session, TopicConfig topic_config, DataCallback&& callback,
             const SubscriberConfig& config = {})
    : subscriber_(
          std::move(session), std::move(topic_config),
          [callback = std::move(callback), this](const MessageMetadata& metadata,
                                                 std::span<const std::byte> buffer) mutable {
            std::call_once(subscriber_check_flag_, [this, &metadata]() { checkTypeInfo(metadata); });

            auto data = std::make_shared<T>();
            serdes::deserialize(buffer, *data);
            callback(metadata, std::move(data));
          },
          serdes::getSerializedTypeInfo<T>(), config) {
  }

private:
  void checkTypeInfo(const MessageMetadata& metadata) {
    const auto serialized_type = serdes::getSerializedTypeInfo<T>().name;
    if (metadata.type_info != serialized_type) {
      heph::log(heph::ERROR, "subscriber type mismatch; terminating", "topic", metadata.topic,
                "subscriber_type", serialized_type, "topic_type", metadata.type_info);
      panic(fmt::format("Topic '{}' is of type '{}', but subscriber expect type '{}'", metadata.topic,
                        metadata.type_info, serialized_type));
    }
  }

private:
  RawSubscriber subscriber_;
  std::once_flag subscriber_check_flag_;
};

/// Create a subscriber for a specific topic.
/// \tparam T The type of the message to subscribe to.
/// \param session The Zenoh session to use.
/// \param topic_config The topic to subscribe to.
/// \param callback The callback to call when a message is received.
/// \param dedicated_callback_thread Whether to use a dedicated thread for the callback. If set to false the
/// callback will be invoked on the Zenoh callback thread.
/// \return A new subscriber.
template <typename T>
[[nodiscard]] auto createSubscriber(zenoh::SessionPtr session, TopicConfig topic_config,
                                    typename Subscriber<T>::DataCallback&& callback,
                                    const SubscriberConfig& config = {}) -> std::unique_ptr<Subscriber<T>> {
  return std::make_unique<Subscriber<T>>(std::move(session), std::move(topic_config), std::move(callback),
                                         config);
}

}  // namespace heph::ipc::zenoh
