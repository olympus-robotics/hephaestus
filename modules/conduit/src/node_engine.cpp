//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/conduit/node_engine.h"

#include <cstddef>
#include <exception>
#include <string>
#include <vector>

#include <fmt/base.h>
#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/node_base.h"
#include "hephaestus/conduit/remote_node_handler.h"
#include "hephaestus/net/endpoint.h"

namespace heph::conduit {

NodeEngine::NodeEngine(const NodeEngineConfig& config)
  : pool_(config.number_of_threads)
  , context_(config.context_config)
  , prefix_(config.prefix)
  , remote_node_handler_(context_, config.endpoints, exception_) {
}

void NodeEngine::run() {
  remote_node_handler_.run();
  try {
    context_.run();
  } catch (...) {
    exception_ = std::current_exception();
  }
  remote_node_handler_.requestStop();
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

auto NodeEngine::endpoints() const -> std::vector<heph::net::Endpoint> {
  return remote_node_handler_.endpoints();
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
      dot_graph += fmt::format("    {}__{} [label=\"{}\", shape=ellipse, color=green];\n", input.node_name,
                               input.name, input.name);
    }

    for (const auto& output : node->outputSpecs()) {
      dot_graph += fmt::format("    {}__{} [label=\"{}\", shape=box, color=blue];\n", output.node_name,
                               output.name, output.name);
    }
    dot_graph += fmt::format("    {}__output [label=\"output\", shape=box, color=blue];\n", node->nodeName());
    dot_graph += "  }\n";
  }

  for (const auto& spec : connection_specs_) {
    dot_graph += fmt::format("  {}__{} -> {}__{};\n", spec.output.node_name, spec.output.name,
                             spec.input.node_name, spec.input.name);
  }

  dot_graph += "}\n";

  std::ranges::replace(dot_graph, '/', '_');
  return dot_graph;
}
}  // namespace heph::conduit
