//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <string>

#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/serdes/type_info.h"

namespace heph::ipc {
class ITopicDatabase {
public:
  virtual ~ITopicDatabase() = default;

  [[nodiscard]] virtual auto getTypeInfo(const std::string& topic) -> const serdes::TypeInfo& = 0;
};

namespace zenoh {
struct Session;
}

[[nodiscard]] auto createZenohTopicDatabase(std::shared_ptr<zenoh::Session> session)
    -> std::unique_ptr<ITopicDatabase>;

}  // namespace heph::ipc
