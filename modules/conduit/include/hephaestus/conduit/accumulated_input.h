//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

#include <exec/task.hpp>
#include <hephaestus/concurrency/repeat_until.h>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/concurrency/channel.h"
#include "hephaestus/concurrency/internal/circular_buffer.h"
#include "hephaestus/conduit/typed_input.h"

namespace heph::conduit {
template <typename T, std::size_t Capacity>
class AccumulatedInput : public TypedInput<T> {
public:
  explicit AccumulatedInput(std::string_view name) : TypedInput<T>(name) {
  }

  auto setValue(T t) -> concurrency::AnySender<void> final {
    return value_channel_.setValue(std::move(t));
  }

  auto value() -> std::vector<T> {
    return buffer_.popAll();
  }

private:
  [[nodiscard]] auto doTrigger(SchedulerT /*scheduler*/) -> concurrency::AnySender<void> final {
    return [this]() -> exec::task<void> {
      while (true) {
        auto value = co_await value_channel_.getValue();
        this->updateTriggerTime();
        while (!buffer_.push(std::move(value))) {
          buffer_.pop();
        }
      }
    }();
    // FIXME:
    /*
    return concurrency::repeatUntil([&]() {
      return value_channel_.getValue() | stdexec::then([this](T value) {
              while (!buffer_.push(std::move(value))) {
                buffer_.pop();
              }
               return false;
             });
    });
    */
  }

  void handleCompleted() final {
  }

  void handleStopped() final {
  }

  void handleError() final {
  }

private:
  std::string_view name_;
  heph::concurrency::Channel<T, 1> value_channel_;
  concurrency::internal::CircularBuffer<T, Capacity> buffer_;
};
}  // namespace heph::conduit
