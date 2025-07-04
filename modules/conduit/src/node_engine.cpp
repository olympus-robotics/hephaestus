//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/node_engine.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/net/accept.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/telemetry/log.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/utils/exception.h"

namespace heph::conduit {

NodeEngine::NodeEngine(const NodeEngineConfig& config)
  : pool_(config.number_of_threads), context_(config.context_config) {
  acceptors_.reserve(config.endpoints.size());

  auto create_acceptor = [&](net::EndpointType type) {
    switch (type) {
#ifndef DISABLE_BLUETOOTH
      case heph::net::EndpointType::BT:
        return heph::net::Acceptor::createL2cap(context_);
#endif
      case heph::net::EndpointType::IPV4:
        return heph::net::Acceptor::createTcpIpV4(context_);
      case heph::net::EndpointType::IPV6:
        return heph::net::Acceptor::createTcpIpV6(context_);
      default:
        heph::panic("Unknown endpoint type");
    }
    __builtin_unreachable();
  };

  for (const auto& endpoint : config.endpoints) {
    acceptors_.push_back(create_acceptor(endpoint.type()));
    acceptors_.back().bind(endpoint);
    acceptors_.back().listen();
  }
}
void NodeEngine::run() {
  for (std::size_t i = 0; i != acceptors_.size(); ++i) {
    scope_.spawn(acceptClients(i));
  }

  context_.run();
  scope_.request_stop();
  stdexec::sync_wait(scope_.on_empty());
  if (exception_) {
    std::rethrow_exception(exception_);
  }
}
void NodeEngine::requestStop() {
  pool_.request_stop();
  context_.requestStop();
}
auto NodeEngine::acceptClients(std::size_t index) -> exec::task<void> {
  const heph::net::Acceptor& server = acceptors_[index];

  while (true) {
    auto client = co_await heph::net::accept(server);
    scope_.spawn(handleClient(std::move(client)));
  }
}
auto NodeEngine::handleClient(heph::net::Socket client) -> exec::task<void> {
  // The protocol is as follows:
  // Datatypes:
  //  - int: int16_t
  //  - string: int length, char[length]
  //
  // Flow:
  // 1. Negotiation:
  //  1. Receive name of node/output (string)
  //  2. Receive expected input type (string)
  //
  // 2. Value loop:
  //  1. Send Data

  std::uint16_t size = 0;
  if (auto res = co_await (heph::net::recvAll(client, std::as_writable_bytes(std::span{ &size, 1 })) |
                           stdexec::stopped_as_optional());
      !res.has_value()) {
    heph::log(heph::WARN, "Connection closed");
    co_return;
  }
  std::string name(size, 0);
  if (auto res = co_await (heph::net::recvAll(client, std::as_writable_bytes(std::span{ name })) |
                           stdexec::stopped_as_optional());
      !res.has_value()) {
    heph::log(heph::WARN, "Connection closed", "name", name);
    co_return;
  }

  if (auto res = co_await (heph::net::recvAll(client, std::as_writable_bytes(std::span{ &size, 1 })) |
                           stdexec::stopped_as_optional());
      !res.has_value()) {
    heph::log(heph::WARN, "Connection closed", "name", name);
    co_return;
  }
  std::string type_info(size, 0);
  if (auto res = co_await (heph::net::recvAll(client, std::as_writable_bytes(std::span{ type_info })) |
                           stdexec::stopped_as_optional());
      !res.has_value()) {
    heph::log(heph::WARN, "Connection closed", "name", name);
    co_return;
  }

  auto it = registered_outputs_.find(name);

  if (it == registered_outputs_.end()) {
    heph::log(heph::ERROR, "Output not found", "name", name);
    co_return;
  }

  auto& [_, entry] = *it;

  if (type_info != entry.type_info) {
    heph::log(heph::ERROR, "Output type mismatch", "name", name, "expected", type_info, "actual",
              entry.type_info);
    co_return;
  }

  heph::log(heph::INFO, "Connected output forwarding client", "name", name);
  entry.publisher_factory(std::move(client));

  co_return;
}

auto NodeEngine::endpoints() const -> std::vector<heph::net::Endpoint> {
  std::vector<heph::net::Endpoint> res;
  res.reserve(acceptors_.size());
  for (const auto& acceptor : acceptors_) {
    res.push_back(acceptor.localEndpoint());
  }
  return res;
}

auto scheduler(NodeEngine& engine) -> concurrency::Context::Scheduler {
  return engine.scheduler();
}
}  // namespace heph::conduit
