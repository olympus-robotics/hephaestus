//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/value_storage.h"

namespace heph::conduit {
template <typename T>
class ValueTrigger {
  using InputTriggerFunctionT = concurrency::AnySender<void> (*)(concurrency::AnySender<T> completion,
                                                                 ValueStorage<T>& value_storage,
                                                                 SchedulerT scheduler,
                                                                 std::optional<ClockT::time_point> deadline);

public:
  template <typename InputTriggerPolicy>
  explicit ValueTrigger(InputTriggerPolicy /*impl*/)
    : trigger_(&InputTriggerPolicy::template Type<T>::trigger) {
  }

  auto operator()(concurrency::AnySender<T> completion, ValueStorage<T>& value_storage, SchedulerT scheduler,
                  std::optional<ClockT::time_point> deadline) -> concurrency::AnySender<void> {
    return trigger_(std::move(completion), value_storage, scheduler, deadline);
  }

private:
  InputTriggerFunctionT trigger_;
};
}  // namespace heph::conduit
