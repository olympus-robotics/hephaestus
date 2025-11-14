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
#include "hephaestus/concurrency/when_all_range.h"
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
  }

  auto setValue(const std::pmr::vector<std::byte>& buffer) -> concurrency::AnySender<void> final {
    if (!this->enabled()) {
      return stdexec::just();
    }
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
  auto setValueImpl(T t) -> concurrency::AnySender<void> {
    if (!this->enabled()) {
      return stdexec::just();
    }
    std::vector<concurrency::AnySender<void>> input_triggers;
    for (auto* input : inputs_) {
      input_triggers.emplace_back(input->setValue(t));
    }

    return concurrency::whenAllRange(std::move(input_triggers));
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
