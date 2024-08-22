//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <span>

#include <zenoh.h>
#include <zenoh.hxx>

#include "hephaestus/ipc/common.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::ipc {

template <class DataType>
using DataCallback = std::function<void(const MessageMetadata&, const std::shared_ptr<DataType>&)>;

template <class Subscriber, class DataType>
[[nodiscard]] auto subscribe(zenoh::SessionPtr session, TopicConfig topic_config,
                             DataCallback<DataType>&& callback,
                             bool dedicated_callback_thread = false) -> std::unique_ptr<Subscriber> {
  auto cb = [callback = std::move(callback)](const MessageMetadata& metadata,
                                             std::span<const std::byte> buffer) mutable {
    auto data = std::make_shared<DataType>();
    heph::serdes::deserialize(buffer, *data);
    callback(metadata, std::move(data));
  };

  return std::make_unique<Subscriber>(std::move(session), std::move(topic_config), std::move(cb),
                                      dedicated_callback_thread);
}

}  // namespace heph::ipc
