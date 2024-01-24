//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include "eolo/serdes/serdes.h"

namespace eolo::ipc {

template <class Publisher, class DataType>
[[nodiscard]] auto publish(Publisher& publisher, const DataType& data) -> bool {
  auto buffer = eolo::serdes::serialize(data);
  return publisher.publish(buffer);
}

}  // namespace eolo::ipc
