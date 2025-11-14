//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exec/task.hpp>

#include "hephaestus/conduit/output_base.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/typed_input.h"

namespace heph::conduit {
template <typename T, std::size_t Capacity = 1>
struct Output;

template <typename T>
struct ForwardingOutput : OutputBase {
  explicit ForwardingOutput(std::string_view name) : OutputBase(name) {
  }

  auto trigger(SchedulerT /*scheduler*/) -> concurrency::AnySender<void> final {
    return stdexec::just();
  }

  template <std::size_t Capacity>
  void forward(Output<T, Capacity>& output);

  void connect(TypedInput<T>& input) {
    inputs_.push_back(&input);
  }

  auto getOutgoing() -> std::vector<std::string> final {
    std::vector<std::string> res;
    for (auto* input : inputs_) {
      res.push_back(input->name());
    }
    return res;
  }
  auto getIncoming() -> std::vector<std::string> final {
    return {};
  }

private:
  template <typename U, std::size_t Capacity>
  friend struct Output;
  std::vector<TypedInput<T>*> inputs_;
};

}  // namespace heph::conduit
