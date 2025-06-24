//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string_view>
#include <utility>

#include <fmt/format.h>
#include <stdexec/execution.hpp>

#include "detail/node_base.h"
#include "hephaestus/conduit/detail/output_connections.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"

namespace heph::conduit {
template <typename T>
class Output {
public:
  template <typename OperationT, typename DataT>
  explicit Output(Node<OperationT, DataT>* node, std::string name) : outputs_(node, std::move(name)) {
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
