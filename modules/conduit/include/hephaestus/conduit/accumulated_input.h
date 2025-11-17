//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/synchronization/mutex.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/concurrency/internal/circular_buffer.h"
#include "hephaestus/conduit/internal/never_stop.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::conduit {
template <typename T, std::size_t Capacity>
class AccumulatedInput : public TypedInput<T> {
public:
  explicit AccumulatedInput(std::string_view name) : TypedInput<T>(name) {
  }

  auto setValue(const std::pmr::vector<std::byte>& buffer) -> concurrency::AnySender<void> final {
    T value{};
    serdes::deserialize(buffer, value);
    return setValue(std::move(value));
  }

  auto setValue(T t) -> concurrency::AnySender<void> final {
    const absl::MutexLock lock{ &mutex_ };
    this->updateTriggerTime();
    while (!buffer_.push(std::move(t))) {
      buffer_.pop();
    }
    return stdexec::just();
  }

  auto value() -> std::vector<T> {
    const absl::MutexLock lock{ &mutex_ };
    return buffer_.popAll();
  }

  [[nodiscard]] virtual auto getTypeInfo() const -> std::string final {
    return serdes::getSerializedTypeInfo<T>().toJson();
  };

private:
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
  std::string_view name_;
  absl::Mutex mutex_;
  concurrency::internal::CircularBuffer<T, Capacity> buffer_ ABSL_GUARDED_BY(mutex_);
};
}  // namespace heph::conduit
