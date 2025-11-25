//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <optional>

#include "hephaestus/concurrency/channel.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/value_storage.h"

namespace heph::conduit {
template <typename T, std::size_t Capacity>
class ValueTrigger {
  using InputTriggerFunctionT = concurrency::AnySender<bool> (*)(
      concurrency::Channel<T, Capacity>& value_channel, ValueStorage<T>& value_storage, SchedulerT scheduler,
      std::optional<ClockT::time_point> deadline);

public:
  template <typename InputTriggerPolicy>
  explicit ValueTrigger(InputTriggerPolicy /*impl*/)
    : trigger_(&InputTriggerPolicy::template Type<T, Capacity>::trigger) {
  }

  auto operator()(concurrency::Channel<T, Capacity>& value_channel, ValueStorage<T>& value_storage,
                  SchedulerT scheduler, std::optional<ClockT::time_point> deadline)
      -> concurrency::AnySender<bool> {
    return trigger_(value_channel, value_storage, scheduler, deadline);
  }

private:
  InputTriggerFunctionT trigger_;
};
}  // namespace heph::conduit
