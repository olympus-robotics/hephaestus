//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/topic.h"

#include <fmt/format.h>

#include "hephaestus/utils/exception.h"

namespace heph::ipc {
TopicConfig::TopicConfig(std::string name) : name(std::move(name)) {
  if (this->name.empty() || this->name.starts_with('/') || this->name.ends_with('/')) {
    throwException<InvalidParameterException>(fmt::format("invalid topic name: '{}'", this->name));
  }
}
}  // namespace heph::ipc
