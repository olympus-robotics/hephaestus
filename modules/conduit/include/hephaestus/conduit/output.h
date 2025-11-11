//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exec/task.hpp>

#include "hephaestus/concurrency/internal/circular_buffer.h"
#include "hephaestus/conduit/forwarding_output.h"
#include "hephaestus/conduit/output_base.h"
#include "hephaestus/conduit/partner_output.h"
#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/utils/exception.h"

namespace heph::conduit {

template <typename T, std::size_t Capacity>
struct Output : public OutputBase {
  explicit Output(std::string_view name) : OutputBase(name) {
  }

  void operator()(T value) {
    if (!buffer_.push(std::move(value))) {
      heph::panic("No space left");
    }
  }

  auto trigger() -> concurrency::AnySender<void> final {
    return triggerImpl();
  }

  void connect(TypedInput<T>& input) {
    inputs_.push_back(&input);
  }

  void connectToPartner(TypedInput<T>& input) {
    partner_outputs_.emplace_back(input);
  }

  auto setPartner(const std::string& prefix, const std::string& partner) -> std::vector<PartnerOutputBase*> {
    std::vector<PartnerOutputBase*> res;
    for (auto& output : partner_outputs_) {
      res.push_back(output.setPartner(prefix, partner));
    }
    return res;
  }

  auto getOutgoing() -> std::vector<std::string> final {
    std::vector<std::string> res;
    for (auto& output : partner_outputs_) {
      res.push_back(output.name());
    }
    for (auto* input : inputs_) {
      res.push_back(input->name());
    }
    for (auto* output : forwarding_outputs_) {
      res.push_back(output->name());
    }
    return res;
  }
  auto getIncoming() -> std::vector<std::string> final {
    std::vector<std::string> res;
    return res;
  }

private:
  auto triggerImpl() -> exec::task<void> {
    while (true) {
      auto value = buffer_.pop();
      if (!value.has_value()) {
        break;
      }
      for (auto& output : partner_outputs_) {
        co_await output.setValue(*value);
      }
      for (auto* input : inputs_) {
        co_await input->setValue(*value);
      }
      for (auto* forwarding : forwarding_outputs_) {
        for (auto* input : forwarding->inputs_) {
          co_await input->setValue(*value);
        }
      }
    }
  }

private:
  template <typename U>
  friend struct ForwardingOutput;

  concurrency::internal::CircularBuffer<T, Capacity> buffer_;
  std::vector<TypedInput<T>*> inputs_;
  std::vector<PartnerOutput<T>> partner_outputs_;
  std::vector<ForwardingOutput<T>*> forwarding_outputs_;
};

template <typename T>
template <std::size_t Capacity>
void ForwardingOutput<T>::forward(Output<T, Capacity>& output) {
  output.forwarding_outputs_.push_back(this);
}
}  // namespace heph::conduit
