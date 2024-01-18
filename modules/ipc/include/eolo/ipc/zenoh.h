//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <sstream>
#include <zenohc.hxx>

namespace eolo::ipc {
inline auto toString(const zenohc::Id& id) -> std::string {
  std::stringstream ss;
  ss << id;
  return ss.str();
}
}  // namespace eolo::ipc
