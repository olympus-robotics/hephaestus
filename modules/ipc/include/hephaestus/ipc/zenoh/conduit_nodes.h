//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <zenoh/api/ext/advanced_subscriber.hxx>
#include <zenoh/api/liveliness.hxx>
#include <zenoh/api/sample.hxx>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/ipc/topic.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc::zenoh {

template <typename T>
struct ConduitSubscriberNode : conduit::Node<ConduitSubscriberNode, T> {};

}  // namespace heph::ipc::zenoh
