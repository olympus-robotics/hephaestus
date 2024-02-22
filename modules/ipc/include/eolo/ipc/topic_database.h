//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <absl/base/thread_annotations.h>

#include "eolo/serdes/type_info.h"

namespace eolo::ipc {
class ITopicDatabase {
public:
  virtual ~ITopicDatabase() = default;

  [[nodiscard]] virtual auto getTypeInfo(const std::string& topic) -> const serdes::TypeInfo& = 0;
};

namespace zenoh {
struct Session;
}

[[nodiscard]] auto
createZenohTopicDatabase(std::shared_ptr<zenoh::Session> session) -> std::unique_ptr<ITopicDatabase>;

}  // namespace eolo::ipc
