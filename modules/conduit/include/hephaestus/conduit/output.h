//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <utility>

#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node.h"

namespace heph::conduit {

class NodeEngine;

template <typename T>
class Output {
public:
  using ResultT = T;
  template <typename OperationT, typename DataT>
  explicit Output(Node<OperationT, DataT>* node, std::string name) : outputs_(node, std::move(name)) {
    if (node != nullptr && node->enginePtr() != nullptr) {
      node->engine().registerOutput(*this);
    }
  }
  auto name() {
    return outputs_.name();
  }

  auto setValue(NodeEngine& engine, T t) {
    return stdexec::just(std::move(t)) | outputs_.propagate(engine);
  }

  template <typename Input>
  void registerInput(Input* input) {
    outputs_.registerInput(input);
  }

private:
  detail::OutputConnections outputs_;
};
}  // namespace heph::conduit
