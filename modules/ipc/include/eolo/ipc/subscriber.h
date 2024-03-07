//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <span>

#include <zenoh.h>
#include <zenohc.hxx>

#include "eolo/ipc/common.h"
#include "eolo/ipc/zenoh/session.h"
#include "eolo/serdes/serdes.h"

namespace eolo::ipc {

template <class DataType>
using DataCallback = std::function<void(const MessageMetadata&, const std::shared_ptr<DataType>&)>;

template <class Subscriber, class DataType>
[[nodiscard]] auto subscribe(zenoh::SessionPtr session, TopicConfig topic_config,
                             DataCallback<DataType>&& callback) -> Subscriber {
  auto cb = [callback = std::move(callback)](const MessageMetadata& metadata,
                                             std::span<const std::byte> buffer) mutable {
    auto data = std::make_shared<DataType>();
    eolo::serdes::deserialize(buffer, *data);
    callback(metadata, std::move(data));
  };

  return Subscriber{ std::move(session), std::move(topic_config), std::move(cb) };
}

}  // namespace eolo::ipc
