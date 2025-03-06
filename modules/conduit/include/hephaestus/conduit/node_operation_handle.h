
#pragma once

#include <vector>

#include <hephaestus/conduit/node_operation.h>

#include "hephaestus/conduit/context.h"

namespace heph::conduit {
struct DataflowGraph;
struct NodeOperationHandle {
  struct NodeVtable {
    void (*run)(void*, DataflowGraph&,
                Context&){ [](void* /*node*/, DataflowGraph& /*g*/, Context& /*context*/) {} };
    std::vector<NodeOperationHandle> (*parents)(void*){ [](void* /*node*/) {
      return std::vector<NodeOperationHandle>{};
    } };
    std::vector<NodeOperationHandle> (*children)(void*){ [](void* /*node*/) {
      return std::vector<NodeOperationHandle>{};
    } };
    std::string_view (*name)(void*){ [](void* /*node*/) -> std::string_view { return "invalid"; } };
    void (*add_child)(void*, NodeOperationHandle) = [](void*, NodeOperationHandle) {};
  };

  NodeOperationHandle() = default;
  template <typename Node>
  explicit NodeOperationHandle(Node* node);
  explicit NodeOperationHandle(std::nullptr_t);

  template <typename Node>
  auto operator=(Node* node) -> NodeOperationHandle&;

  void run(DataflowGraph& g, Context& context) const {
    vtable->run(node, g, context);
  }

  [[nodiscard]] auto parents() const {
    return vtable->parents(node);
  }

  [[nodiscard]] auto children() const {
    return vtable->children(node);
  }

  [[nodiscard]] auto name() const {
    return vtable->name(node);
  }

  void addChild(NodeOperationHandle parent) const {
    vtable->add_child(node, parent);
  }

  void* node{ nullptr };
  NodeVtable const* vtable{ nullptr };
};

template <typename Node>
inline constexpr NodeOperationHandle::NodeVtable NODE_VTABLE{
  .run = [](void* node, DataflowGraph& g, Context& context) { static_cast<Node*>(node)->run(g, context); },
  .parents = [](void* node) { return static_cast<Node*>(node)->parents(); },
  .children = [](void* node) { return static_cast<Node*>(node)->children(); },
  .name = [](void* node) { return static_cast<Node*>(node)->getName(); },
  .add_child = [](void* node,
                  NodeOperationHandle parent) { return static_cast<Node*>(node)->addChild(parent); }
};

inline constexpr NodeOperationHandle::NodeVtable DEFAULT_NODE_VTABLE{};

template <typename Node>
NodeOperationHandle::NodeOperationHandle(Node* node) : node(node), vtable(&NODE_VTABLE<Node>) {
}
inline NodeOperationHandle::NodeOperationHandle(std::nullptr_t) : vtable(&DEFAULT_NODE_VTABLE) {
}

template <typename Node>
auto NodeOperationHandle::operator=(Node* n) -> NodeOperationHandle& {
  node = n;
  vtable = &NODE_VTABLE<Node>;
  return *this;
}
}  // namespace heph::conduit
