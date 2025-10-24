//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <absl/synchronization/mutex.h>
#include <exec/async_scope.hpp>
#include <exec/task.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/concurrency/context_scheduler.h"
#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/partner_output.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"

namespace heph::conduit {

struct AcceptorConfig {
  std::vector<heph::net::Endpoint> endpoints;
  absl::flat_hash_map<std::string, heph::net::Endpoint> partners;
};

class Acceptor {
public:
  explicit Acceptor(const AcceptorConfig& config);
  ~Acceptor();

  void join();
  auto endpoints() const -> std::vector<heph::net::Endpoint>;
  void addPartner(std::string name, heph::net::Endpoint endpoint);
  void requestStop();
  auto acceptClient(std::size_t index) -> exec::task<void>;
  void spawn(const std::vector<PartnerOutputBase*>& outputs);
  void setInputs(std::vector<BasicInput*> typed_inputs);

private:
  auto handleClient(net::Socket client, std::uint64_t type) -> exec::task<void>;

private:
  absl::Mutex mutex_;
  exec::async_scope scope_;
  heph::concurrency::Context context_{ {} };
  std::vector<heph::net::Acceptor> acceptors_;
  std::exception_ptr exception_;
  std::vector<BasicInput*> typed_inputs_;
  absl::flat_hash_map<std::string, heph::net::Endpoint> partners_;
  std::thread thread_;
};
}  // namespace heph::conduit
