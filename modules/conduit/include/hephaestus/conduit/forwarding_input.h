//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/internal/never_stop.h"
#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/serdes/serdes.h"

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

  auto setValue(const std::pmr::vector<std::byte>& buffer) -> concurrency::AnySender<void> final {
    T value{};
    serdes::deserialize(buffer, value);
    return setValueImpl(std::move(value));
  }

  void forward(TypedInput<T>& input) {
    inputs_.push_back(&input);
  }

  auto getOutgoing() -> std::vector<std::string> final {
    std::vector<std::string> res;
    for (auto* input : inputs_) {
      res.push_back(input->name());
    }
    return res;
  }

  [[nodiscard]] auto getTypeInfo() const -> std::string final {
    for (auto* input : inputs_) {
      return input->getTypeInfo();
    }
    abort();
    return "";
  };

private:
  auto setValueImpl(T t) -> exec::task<void> {
    for (auto* input : inputs_) {
      if (input->enabled()) {
        // fmt::println(stderr, "{} forward to {}", this->name(), input->name());
        co_await input->setValue(t);
      }
    }
  }

  [[nodiscard]] auto doTrigger(SchedulerT /*scheduler*/) -> concurrency::AnySender<bool> final {
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
