//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <rfl/to_view.hpp>

#include "hephaestus/conduit/graph.h"
#include "hephaestus/conduit/node.h"

namespace heph::conduit {
namespace internal {
struct Visualization {
  struct Edge {
    std::size_t source;
    std::size_t sink;
  };
  std::vector<Edge> edge;
  std::unordered_map<std::string, std::size_t> ids;
  std::size_t id{ 0 };

  auto getId(const std::string& name) {
    auto it = ids.find(name);
    if (it != ids.end()) {
      return it->second;
    }
    auto this_id = id;
    ++id;
    ids.emplace(name, this_id);
    return this_id;
  }

  void addEdge(const std::string& source, const std::string& sink) {
    auto source_id = getId(source);
    auto sink_id = getId(sink);
    edge.emplace_back(source_id, sink_id);
  }
};
inline auto strip(const std::string& name) -> std::string {
  auto pos = name.find_last_of('/');
  if (pos == std::string::npos) {
    return name;
  }
  return name.substr(pos + 1);
}

template <typename NodeDescription>
auto dotGraphImpl(Visualization& visualization, heph::conduit::Node<NodeDescription>& root) -> std::string {
  std::string res;
  auto pre_visit = [&](auto& node) {
    auto id = visualization.getId(node->name());
    res += fmt::format("subgraph cluster_{} {{\n", id);
    res += fmt::format("label = \"{}\";\n", strip(node->name()));
    res += fmt::format("subgraph ports{} {{\n", id);
    res += fmt::format("subgraph cluster_inputs{} {{\n", id);
    res += fmt::format("label = \"Inputs\";\n");
    auto inputs = rfl::to_view(node->inputs);
    inputs.apply([&](const auto& input) {
      auto id = visualization.getId(input.value()->name());
      res += fmt::format("{} [label = \"{}\", shape = ellipse];\n", id, strip(input.name()));
      for (const auto& destination : input.value()->getOutgoing()) {
        visualization.addEdge(input.value()->name(), destination);
      }
      for (const auto& destination : input.vaue()->getIncoming()) {
        visualization.addEdge(destination, input.value()->name());
      }
    });
    res += "}\n";
    res += fmt::format("subgraph cluster_outputs{} {{\n", id);
    res += fmt::format("label = \"Outputs\";\n");

    auto outputs = rfl::to_view(node->inputs);
    rfl::apply([&](const auto& output) {
      auto id = visualization.getId(output.value()->name());
      res += fmt::format("{} [label = \"{}\", shape = box];\n", id, strip(output.value()->name()));
      for (const auto& destination : output.value()->getOutgoing()) {
        visualization.addEdge(output.value()->name(), destination);
      }
      for (const auto& destination : output.value()->getIncoming()) {
        visualization.addEdge(destination, output.value()->name());
      }
    });
    res += "}\n";
    res += "}\n";
  };
  auto post_visit = [&](auto& /*node*/) { res += "}\n"; };

  auto id = visualization.getId(root->prefix());
  res += fmt::format("subgraph cluster_node{} {{\n", id);
  res += fmt::format("label = \"{}\";\n", root->prefix());
  traverse(root, pre_visit, post_visit);
  res += "}\n";
  return res;
}
}  // namespace internal
template <typename... NodeDescription>
auto dotGraph(heph::conduit::Node<NodeDescription>&... roots) -> std::string {
  std::string res;
  internal::Visualization vis;

  res += "digraph Robot {\n";
  res += "rankdir = LR;\n";

  res += (internal::dotGraphImpl(vis, roots) + ...);

  for (auto [source, sink] : vis.edge) {
    res += fmt::format("{} -> {};\n", source, sink);
  }
  res += "}\n";

  return res;
}
}  // namespace heph::conduit
