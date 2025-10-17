//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exec/task.hpp>

#include "hephaestus/conduit/output.h"
#include "hephaestus/conduit/output_base.h"
#include "hephaestus/conduit/typed_input.h"

namespace heph::conduit {

template <typename T>
struct ForwardingOutput : OutputBase {
  explicit ForwardingOutput(std::string_view name) : OutputBase(name) {
  }

  auto trigger() -> concurrency::AnySender<void> final {
    return stdexec::just();
  }

  void forward(Output<T>& output) {
    outputs_.push_back(&output);
  }

  void connect(TypedInput<T>& input) {
    for (auto* output : outputs_) {
      output->connect(input);
    }
  }

private:
  std::vector<Output<T>*> outputs_;
};

}  // namespace heph::conduit
