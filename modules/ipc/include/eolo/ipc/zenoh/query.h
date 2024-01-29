//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <zenohc.hxx>

namespace eolo::ipc::zenoh {

struct QueryResponse {
  std::string topic;
  std::string value;
};

[[nodiscard]] auto query(zenohc::Session& session, const std::string& topic,
                         const std::string& value) -> std::vector<QueryResponse>;

}  // namespace eolo::ipc::zenoh
