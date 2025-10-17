//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <utility>
#include <vector>

#include <exec/task.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/internal/never_stop.h"
#include "hephaestus/conduit/typed_input.h"

namespace heph::conduit {
template <typename T>
class ForwardingInput : public TypedInput<T> {
public:
  explicit ForwardingInput(std::string_view name) : TypedInput<T>(name) {
  }

  auto setValue(T t) -> concurrency::AnySender<void> final {
    return setValueImpl(std::move(t));
    // FIXME:
    // return concurrency::when_all(...);
  }

  void forward(TypedInput<T>& input) {
    inputs_.push_back(&input);
  }

private:
  auto setValueImpl(T t) -> exec::task<void> {
    for (auto* input : inputs_) {
      co_await input->setValue(t);
    }
  }

  [[nodiscard]] auto doTrigger(SchedulerT /*scheduler*/) -> concurrency::AnySender<void> final {
    return internal::NeverStop{};
  }

  void handleCompleted() final {
  }

  void handleStopped() final {
  }

  void handleError() final {
  }

private:
  std::vector<TypedInput<T>*> inputs_;
};
}  // namespace heph::conduit
