//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <hephaestus/conduit/node_base.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/clock.h"
#include "hephaestus/conduit/scheduler.h"

namespace heph::conduit {

static inline constexpr auto OVERWRITE_POLICY = std::size_t(-1);

/// A trigger can return with either ready (value), with an exception or stopped.
using InputTriggerCompletionSignatures =
    stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                   stdexec::set_stopped_t()>;

class BasicInput;
class InputTrigger;

template <typename Sender>
concept InputTriggerSender =
    !std::is_same_v<std::decay_t<Sender>, InputTrigger> &&
    (stdexec::sender_of<Sender, stdexec::set_value_t(), concurrency::AnyEnv> ||
     stdexec::sender_of<Sender, stdexec::set_stopped_t(), concurrency::AnyEnv> ||
     stdexec::sender_of<Sender, stdexec::set_error_t(std::exception_ptr), concurrency::AnyEnv>);
;

namespace internal {
template <typename Receiver>
struct TriggerReceiver {
  using receiver_concept = stdexec::receiver_t;
  BasicInput* input;
  Receiver receiver;
  //  NOLINTBEGIN (readability-identifier-naming) - wrapping stdexec interface
  void set_value(bool completed) noexcept;
  void set_stopped() noexcept;
  void set_error(std::exception_ptr ptr) noexcept;

  [[nodiscard]] auto get_env() const noexcept {
    return stdexec::get_env(receiver);
  }
  // NOLINTEND
};
}  // namespace internal

/// Type-erased Sender used as the return value of \ref BasicInput::trigger.
///
/// On completion, it updates the inputs trigger time.
class InputTrigger {
public:
  using sender_concept = stdexec::sender_t;
  using completion_signatures = InputTriggerCompletionSignatures;

  InputTrigger(BasicInput* input, concurrency::AnySender<bool>&& sender)
    : input_(input), sender_(std::move(sender)) {
  }

  ~InputTrigger() = default;
  InputTrigger(InputTrigger&& other) = default;
  auto operator=(InputTrigger&& other) -> InputTrigger& = default;
  InputTrigger(const InputTrigger&) = delete;
  auto operator=(const InputTrigger&) -> InputTrigger& = delete;

  template <stdexec::receiver_of<completion_signatures> Receiver>
  auto connect(Receiver&& receiver) && {
    return stdexec::connect(std::move(sender_),
                            internal::TriggerReceiver{ input_, std::forward<Receiver>(receiver) });
  }

private:
  BasicInput* input_;
  concurrency::AnySender<bool> sender_;
};

// APIDOC: begin
/// Providing a virtual base class, usable for type erasure to allow for extensions.
/// @verbatim embed:rst
/// See :ref:`heph.conduit.concepts.inputs`
/// @endverbatim
class BasicInput {
public:
  using SenderT = concurrency::AnySender<bool>;
  /// Each input is named. Therefor, basic inputs can only be constructed with a name.
  ///
  /// Initializes \ref lastTriggerTime() to epoch time.
  explicit BasicInput(std::string_view name);

  virtual ~BasicInput() = default;

  /// Each input needs to provide a trigger which signals its completion. This trigger can
  /// complete in the following ways:
  ///  - Ready: The input was triggered successfully
  ///  - Stopped: The input trigger returned, but fulfilling the input policies was not possible
  ///
  /// This will call \ref doTrigger
  ///
  /// @param scheduler Used for triggers to schedule their completions (for example timeouts or IO related
  ///        tasks)
  /// @return A Sender representing the completion of an input signal. This sender doesn't complete with a
  ///         value, potential values need to be queried from the specific input implementations
  [[nodiscard]] auto trigger(SchedulerT scheduler) -> InputTrigger;

  /// Override to provide the input trigger
  [[nodiscard]] virtual auto doTrigger(SchedulerT scheduler) -> SenderT = 0;

  /// Retrieve the name of input. This is not usable as a identifier as it doesn't include the whole path
  /// leading to this input.
  [[nodiscard]] auto name() const -> std::string;

  /// In order for algorithms to react on stale inputs, this function can be used.
  ///
  /// @return The time point at which the last trigger event occurred
  [[nodiscard]] auto lastTriggerTime() const -> ClockT::time_point;

  [[nodiscard]] virtual auto getTypeInfo() const -> std::string;

  virtual auto setValue(const std::pmr::vector<std::byte>& /*buffer*/) -> concurrency::AnySender<void>;

  virtual void handleCompleted() = 0;
  virtual void handleError();
  virtual void handleStopped();

  void updateTriggerTime();

  void setNode(NodeBase& node) {
    node_ = &node;
  }

  auto enabled() const -> bool;

  void enable();

  void disable();

  virtual auto getIncoming() -> std::vector<std::string>;

  virtual auto getOutgoing() -> std::vector<std::string>;

protected:
  auto node() -> NodeBase* {
    return node_;
  }

private:
  template <typename Receiver>
  friend struct internal::TriggerReceiver;
  void onCompleted();

private:
  NodeBase* node_{ nullptr };
  std::string_view name_;
  ClockT::time_point last_trigger_time_;
  std::atomic<bool> enabled_{ true };
};
// APIDOC: end

namespace internal {
template <typename Receiver>
inline void TriggerReceiver<Receiver>::set_value(bool completed) noexcept {
  if (completed) {
    input->onCompleted();
  } else {
    input->handleStopped();
  }
  stdexec::set_value(std::move(receiver));
}
template <typename Receiver>
inline void TriggerReceiver<Receiver>::set_error(std::exception_ptr ptr) noexcept {
  input->handleError();
  stdexec::set_error(std::move(receiver), std::move(ptr));
}

template <typename Receiver>
inline void TriggerReceiver<Receiver>::set_stopped() noexcept {
  input->handleStopped();
  stdexec::set_stopped(std::move(receiver));
}

}  // namespace internal

}  // namespace heph::conduit
