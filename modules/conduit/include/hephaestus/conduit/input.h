//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/concurrency/channel.h"
#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/input_policy.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/conduit/typed_input.h"
#include "hephaestus/conduit/value_storage.h"
#include "hephaestus/conduit/value_trigger.h"
#include "hephaestus/serdes/serdes.h"

namespace heph::conduit {

static inline constexpr auto OVERWRITE_POLICY = std::size_t(-1);

/// `Input` is representing the set of typed inputs which are the incoming edges into a node in the
/// execution graph.
template <typename T, std::size_t QueueDepth = 1>
class Input : public TypedInput<T> {
  using typename TypedInput<T>::SetValueSenderT;
  using BasicInput::SenderT;

  static constexpr bool OVERWRITE = QueueDepth == OVERWRITE_POLICY;
  static constexpr auto QUEUE_DEPTH = OVERWRITE ? 1 : QueueDepth;

public:
  /// @tparam Policy type
  /// @param name Each input has a name
  /// @param policy Each input as a policy
  template <typename ValueStoragePolicy = ResetValuePolicy, typename TriggerPolicy = BlockingTrigger>
  explicit Input(std::string_view name, InputPolicy<ValueStoragePolicy, TriggerPolicy> policy = {})
    : TypedInput<T>(name)
    , value_storage_(policy.storage_policy.template bind<T>())
    , value_trigger_(policy.trigger_policy.template bind<T>()) {
  }

  auto setValue(T t) -> SetValueSenderT final {
    if (!this->enabled()) {
      return stdexec::just();
    }
    if constexpr (OVERWRITE) {
      value_channel_.setValueOverwrite(std::move(t));
      return stdexec::just();
    } else {
      return value_channel_.setValue(std::move(t));
    }
  }

  [[nodiscard]] auto hasValue() const -> bool {
    return value_storage_.hasValue();
  }

  auto value() -> T {
    return value_storage_.value();
  }

  template <typename... Ts>
  auto valueOr(Ts&&... ts) -> T {
    if (hasValue()) {
      return value_storage_.value();
    }
    return T{ std::forward<Ts>(ts)... };
  }

  auto optionalValue() -> std::optional<T> {
    return hasValue() ? std::optional{ value() } : std::nullopt;
  }

  void setTimeout(ClockT::duration timeout) {
    timeout_.emplace(timeout);
  }

  [[nodiscard]] virtual auto getTypeInfo() const -> std::string final {
    return serdes::getSerializedTypeInfo<T>().toJson();
  };

  auto setValue(const std::pmr::vector<std::byte>& buffer) -> concurrency::AnySender<void> final {
    if (!this->enabled()) {
      return stdexec::just();
    }
    T value{};
    serdes::deserialize(buffer, value);
    return setValue(std::move(value));
  }

private:
  auto doTrigger(SchedulerT scheduler) -> SenderT final {
    std::optional<ClockT::time_point> deadline;
    if (timeout_.has_value()) {
      deadline = ClockT::now() + *timeout_;
    }
    return value_trigger_(value_channel_.getValue(), value_storage_, scheduler, deadline);
  }

  void handleCompleted() final {
  }

  void handleStopped() final {
  }

  void handleError() final {
  }

private:
  heph::concurrency::Channel<T, QUEUE_DEPTH> value_channel_;
  ValueStorage<T> value_storage_;
  ValueTrigger<T> value_trigger_;
  std::optional<ClockT::duration> timeout_;
};

}  // namespace heph::conduit
