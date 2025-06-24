//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/topic.h"

#include <string>
#include <utility>

#include <fmt/format.h>

#include "hephaestus/utils/exception.h"

namespace heph::ipc {
TopicConfig::TopicConfig(std::string topic_name) : name(std::move(topic_name)) {
  if (name.empty() || name.starts_with('/') || name.ends_with('/')) {
    panic("invalid topic name: '{}'", name);
  }
}
}  // namespace heph::ipc
