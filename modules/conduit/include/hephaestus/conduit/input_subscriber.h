//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

#include <fmt/format.h>

#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/node_handle.h"

namespace heph::conduit {

template <typename InputT>
class InputSubscriber {
  struct Node : heph::conduit::Node<Node, InputSubscriber*> {
    static auto name(const Node& self) {
      return self.data()->name();
    }

    static auto trigger(Node& self) {
      return self.data()->input_->peek();
    }

    static auto execute(auto value) {
      return value;
    }
  };

public:
  InputSubscriber(NodeEngine& engine, InputT& input)
    : input_(&input), node_(engine.createNode<Node>(this)), name_([&] {
      // The name is /<prefix>/<node>/<input>. We need to remove the prefix
      // and then append 'subscriber'
      auto prefix = engine.prefix();
      auto stripped_name = input.name().substr(prefix.size());
      return fmt::format("{}/subscriber", stripped_name);
    }()) {
  }

  auto output() -> NodeHandle<Node>& {
    return node_;
  }

  auto name() {
    return name_;
  }

private:
  InputT* input_;
  NodeHandle<Node> node_;
  std::string name_;
};
}  // namespace heph::conduit
