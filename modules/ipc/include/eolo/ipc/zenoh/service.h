//================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <zenohc.hxx>

#include "eolo/ipc/zenoh/session.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

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

}  // namespace eolo::ipc::zenoh
