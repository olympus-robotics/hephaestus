//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include "eolo/ipc/common.h"

namespace eolo::ipc::zenoh {

struct PublisherInfo {
  std::string topic;
};

[[nodiscard]] auto getListOfPublishers(Config&& config) -> std::vector<PublisherInfo>;

void printPublisherInfo(const PublisherInfo& info);

}  // namespace eolo::ipc::zenoh
