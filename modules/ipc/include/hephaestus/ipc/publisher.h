//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/serdes/serdes.h"

namespace heph::ipc {

template <class Publisher, class DataType>
[[nodiscard]] auto publish(Publisher& publisher, const DataType& data) -> bool {
  auto buffer = serdes::serialize(data);
  return publisher.publish(buffer);
}

}  // namespace heph::ipc
