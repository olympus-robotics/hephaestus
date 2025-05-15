//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

#include <stdexec/execution.hpp>

#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"

namespace heph::conduit {
template <typename T>
class Output {
public:
  template <typename OperationT>
  explicit Output(Node<OperationT>* node, std::string name = "") : node_(node), name_(std::move(name)) {
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
  void* node_;
  std::string name_;
};
}  // namespace heph::conduit
