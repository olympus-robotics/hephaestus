//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <utility>

#include <hephaestus/conduit/detail/node_base.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/utils/utils.h"

namespace heph::conduit {

class NodeEngine;

template <typename T>
class Output {
public:
  using ResultT = T;
  template <typename OperationT, typename DataT>
  explicit Output(Node<OperationT, DataT>* node, std::string name)
    : name_(std::move(name)), outputs_(node, name_) {
    if (node != nullptr) {
      node->addOutputSpec([this, node] {
        return detail::OutputSpecification{
          .name = name_,
          .node_name = node->nodeName(),
          .type = heph::utils::getTypeName<T>(),
        };
      });
      if (node->enginePtr() != nullptr) {
        node->engine().registerOutput(*this);
      }
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
  std::string name_;
  detail::OutputConnections outputs_;
};
}  // namespace heph::conduit
