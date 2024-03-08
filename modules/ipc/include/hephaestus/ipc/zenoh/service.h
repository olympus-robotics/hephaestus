//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <zenohc.hxx>

#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/ipc/zenoh/utils.h"

namespace heph::ipc::zenoh {

struct QueryRequest {
  std::string topic;
  std::string parameters;
  std::string value;
};

class Service {
public:
  using Callback = std::function<std::string(const QueryRequest&)>;
  Service(SessionPtr session, std::string topic, Callback&& callback);

private:
  SessionPtr session_;
  std::unique_ptr<zenohc::Queryable> queryable_;

  std::string topic_;
  Callback callback_;
};

}  // namespace heph::ipc::zenoh
