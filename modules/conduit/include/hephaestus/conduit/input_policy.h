//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <optional>
#include <utility>

#include <exec/when_any.hpp>
#include <hephaestus/utils/exception.h>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/value_storage.h"

namespace heph::conduit {

template <typename ValueStoragePolicy, typename TriggerPolicy>
struct InputPolicy {
  ValueStoragePolicy storage_policy;
  TriggerPolicy trigger_policy;
};

struct ResetValuePolicy {
  template <typename T>
  class Type {
  public:
    [[nodiscard]] auto hasValue() const -> bool {
      return value_.has_value();
    }

    [[nodiscard]] auto value() -> T {
      std::optional<T> res = std::nullopt;
      std::swap(res, value_);
      return res.value();
    }

    void setValue(T&& t) {
      heph::panicIf(value_.has_value(),
                    "Storage already contains a value. Did you forget to consume an input?");
      value_.emplace(std::move(t));
    }

  private:
    std::optional<T> value_;
  };

  template <typename T>
  [[nodiscard]] auto bind() const -> Type<T> {
    return {};
  }
};

struct BlockingTrigger {
  template <typename T>
  struct Type {
    static auto trigger(concurrency::AnySender<T> completion, ValueStorage<T>& value_storage,
                        SchedulerT /*scheduler*/, std::optional<ClockT::time_point> /*deadline*/)
        -> concurrency::AnySender<void> {
      return std::move(completion) |
             stdexec::then([&value_storage](T&& value) -> void { value_storage.setValue(std::move(value)); });
    }
  };
  template <typename T>
  auto bind() -> Type<T> {
    return {};
  }
};

using BlockingInputPolicy = InputPolicy<ResetValuePolicy, BlockingTrigger>;

struct KeepLastValuePolicy {
  template <typename T>
  class Type {
  public:
    [[nodiscard]] auto hasValue() const -> bool {
      return value_.has_value();
    }

    [[nodiscard]] auto value() -> T {
      return value_.value();
    }

    void setValue(T&& t) {
      value_.emplace(std::move(t));
    }

  private:
    std::optional<T> value_;
  };

  template <typename T>
  [[nodiscard]] auto bind() const -> Type<T> {
    return {};
  }
};

struct DeadlineTrigger {
  template <typename T>
  struct Type {
    static auto trigger(concurrency::AnySender<T> completion, ValueStorage<T>& value_storage,
                        SchedulerT scheduler, std::optional<ClockT::time_point> deadline)
        -> concurrency::AnySender<void> {
      heph::panicIf(!deadline.has_value(), "DeadlineTrigger called without setting a timeout");
      return exec::when_any(scheduler.scheduleAt(*deadline),
                            std::move(completion) | stdexec::then([&value_storage](T&& value) -> void {
                              value_storage.setValue(std::move(value));
                            }));
    }
  };
  template <typename T>
  auto bind() -> Type<T> {
    return {};
  }
};
using BestEffortInputPolicy = InputPolicy<KeepLastValuePolicy, DeadlineTrigger>;

}  // namespace heph::conduit
