//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

namespace heph::conduit {

template <typename NodeT>
class NodeHandle {
public:
  explicit NodeHandle(NodeT* node) : node_(node) {
  }
  ~NodeHandle() = default;
  NodeHandle(const NodeHandle&) = default;
  auto operator=(const NodeHandle&) -> NodeHandle& = default;
  NodeHandle(NodeHandle&&) = default;
  auto operator=(NodeHandle&&) -> NodeHandle& = default;

  auto operator*() -> NodeT& {
    return *node_;
  }

  auto operator->() -> NodeT* {
    return node_;
  }

  auto get() -> NodeT& {
    return *node_;
  }

  auto get() const -> const NodeT& {
    return *node_;
  }

private:
  NodeT* node_;
};
}  // namespace heph::conduit
