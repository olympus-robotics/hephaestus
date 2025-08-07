//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/remote_node_handler.h"

#include <cstdio>
#include <exception>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/context.h"
#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/net/accept.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/recv.h"
#include "hephaestus/net/socket.h"
#include "hephaestus/utils/exception.h"

namespace heph::conduit {

RemoteNodeHandler::RemoteNodeHandler(concurrency::Context& context,
                                     const std::vector<heph::net::Endpoint>& endpoints,
                                     std::exception_ptr& exception)
  : exception_(exception) {
  acceptors_.reserve(endpoints.size());

  auto create_acceptor = [&](net::EndpointType type) {
    switch (type) {
#ifndef DISABLE_BLUETOOTH
      case heph::net::EndpointType::BT:
        return heph::net::Acceptor::createL2cap(context);
#endif
      case heph::net::EndpointType::IPV4:
        return heph::net::Acceptor::createTcpIpV4(context);
      case heph::net::EndpointType::IPV6:
        return heph::net::Acceptor::createTcpIpV6(context);
      default:
        heph::panic("Unknown endpoint type");
    }
    __builtin_unreachable();
  };

  for (const auto& endpoint : endpoints) {
    acceptors_.push_back(create_acceptor(endpoint.type()));
    acceptors_.back().bind(endpoint);
    acceptors_.back().listen();
  }
}
RemoteNodeHandler::~RemoteNodeHandler() {
  stdexec::sync_wait(scope_.on_empty());
}

auto RemoteNodeHandler::endpoints() const -> std::vector<heph::net::Endpoint> {
  std::vector<heph::net::Endpoint> res;
  res.reserve(acceptors_.size());
  for (const auto& acceptor : acceptors_) {
    res.push_back(acceptor.localEndpoint());
  }
  return res;
}

void RemoteNodeHandler::run() {
  for (std::size_t i = 0; i != acceptors_.size(); ++i) {
    scope_.spawn(acceptClients(i));
  }
}

void RemoteNodeHandler::requestStop() {
  scope_.request_stop();
}

auto RemoteNodeHandler::acceptClients(std::size_t index) -> exec::task<void> {
  const heph::net::Acceptor& server = acceptors_[index];

  while (true) {
    try {
      auto client = co_await heph::net::accept(server);
      RemoteNodeType type{};
      co_await heph::net::recvAll(client, std::as_writable_bytes(std::span{ &type, 1 }));
      scope_.spawn(handleClient(std::move(client), type));
    } catch (...) {
      exception_ = std::current_exception();
      co_return;
    }
  }
}

namespace {
auto recvNameInfo(heph::net::Socket* client) -> exec::task<std::tuple<std::string, std::string>> {
  auto name = co_await internal::recv<std::string>(*client);
  auto type_info = co_await internal::recv<std::string>(*client);

  co_return { std::move(name), std::move(type_info) };
}

auto checkTypeInfo(heph::net::Socket* client, const std::string* name, std::string received_type_info,
                   std::string* entry_type_info) -> exec::task<bool> {
  if (received_type_info != *entry_type_info) {
    heph::log(heph::ERROR, "Type mismatch", "name", *name, "expected", received_type_info, "actual",
              *entry_type_info, "client", client->remoteEndpoint());
    auto error = fmt::format("Type mismatch: Expected {}, got {}", *entry_type_info, received_type_info);
    co_await internal::send(*client, error);
    co_return false;
  }
  co_return true;
}
}  // namespace

auto RemoteNodeHandler::handleClient(heph::net::Socket client, RemoteNodeType type) -> exec::task<void> {
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
  try {
    // NOLINTNEXTLINE (readability-static-accessed-through-instance)
    auto [name, type_info] = co_await recvNameInfo(&client);

    auto it = client_handlers_.find(name);

    if (it == client_handlers_.end()) {
      const std::string error = fmt::format("{} client handler not found", type.type);
      heph::log(heph::ERROR, error, "name", name);
      // NOLINTNEXTLINE (readability-static-accessed-through-instance)
      co_await internal::send(client, error);

      co_return;
    }

    auto& [_, entry] = *it;

    // NOLINTNEXTLINE (readability-static-accessed-through-instance)
    if (!co_await checkTypeInfo(&client, &name, std::move(type_info), &entry.type_info)) {
      co_return;
    }

    // NOLINTNEXTLINE (readability-static-accessed-through-instance)
    co_await internal::send(client, CONNECT_SUCCESS);

    heph::log(heph::INFO, "Client connected", "name", name, "type", type.type, "reliable", type.reliable,
              "client", client.remoteEndpoint());
    entry.factory(std::move(client), type.reliable);
  } catch (std::exception& exception) {
    heph::log(heph::ERROR, "Output subscriber disconnected", "exception", exception.what());
  }
}

}  // namespace heph::conduit
