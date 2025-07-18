//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/node_engine.h"

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/net/accept.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/endpoint.h"
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

  auto name = co_await internal::recv<std::string>(client);
  auto type_info = co_await internal::recv<std::string>(client);

  auto it = registered_outputs_.find(name);

  if (it == registered_outputs_.end()) {
    const std::string error = "Output not found";
    heph::log(heph::ERROR, error, "name", name);
    co_await internal::send(client, error);

    co_return;
  }

  auto& [_, entry] = *it;

  if (type_info != entry.type_info) {
    heph::log(heph::ERROR, "Type mismatch", "name", name, "expected", type_info, "actual", entry.type_info,
              "client", client.remoteEndpoint());
    auto error = fmt::format("Type mismatch: Expected {}, got {}", entry.type_info, type_info);
    co_await internal::send(client, error);
    co_return;
  }

  // No error, just send back a "zero" string. to acknowledge the full negotiation.
  static constexpr std::string_view SUCCESS{ "success" };
  co_await internal::send(client, SUCCESS);

  heph::log(heph::INFO, "Connected subscriber", "name", name, "client", client.remoteEndpoint());
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

auto NodeEngine::getDotGraph() const -> std::string {
  fmt::println("Node: {}, connections: {}", nodes_.size(), connection_specs_.size());

  std::string dot_graph = "digraph {\n";
  std::size_t counter = 0;
  for (const auto& node : nodes_) {
    dot_graph += fmt::format("  subgraph cluster_{} {{\n", counter++);
    dot_graph += fmt::format("    label=\"{}\";\n", node->nodeName());
    for (const auto& input : node->inputSpecs()) {
      dot_graph += fmt::format("    {}_{} [label=\"{}\", shape=ellipse, color=green];\n", input.node_name,
                               input.name, input.name);
    }

    for (const auto& output : node->outputSpecs()) {
      dot_graph += fmt::format("    {}_{} [label=\"{}\", shape=box, color=blue];\n", output.node_name,
                               output.name, output.name);
    }
    dot_graph += fmt::format("    {}_output [label=\"output\", shape=box, color=blue];\n", node->nodeName());
    dot_graph += "  }\n";
  }

  for (const auto& spec : connection_specs_) {
    dot_graph += fmt::format("  {}_{} -> {}_{};\n", spec.output.node_name, spec.output.name,
                             spec.input.node_name, spec.input.name);
  }

  dot_graph += "}\n";
  return dot_graph;
}
}  // namespace heph::conduit
