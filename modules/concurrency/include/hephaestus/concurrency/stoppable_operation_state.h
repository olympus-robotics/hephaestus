//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <tuple>
#include <utility>

#include <absl/synchronization/mutex.h>
#include <stdexec/execution.hpp>

#include "hephaestus/utils/unique_function.h"
#include "hephaestus/error_handling/panic.h"

namespace heph::concurrency {
template <typename Receiver, typename... Values>
struct StoppableOperationState {
  enum State {
    STARTING,
    STARTED,
    COMPLETED,
    STOPPED,
    ERROR,
  };
  class ToStartedTransition {
  public:
    explicit ToStartedTransition(StoppableOperationState* self) noexcept : self_(self) {
    }
    ToStartedTransition(ToStartedTransition&& other) noexcept : self_(std::exchange(other.self_, nullptr)) {
    }
    auto operator=(ToStartedTransition&& other) noexcept -> ToStartedTransition& {
      self_ = std::exchange(other.self_, nullptr);

      return *this;
    }
    ~ToStartedTransition() noexcept {
      if (self_ == nullptr) {
        return;
      }
      State current_state = STARTING;
      {
        absl::MutexLock l{ &self_->mutex };
        if (self_->state == STARTING) {
          self_->state = STARTED;
          return;
        }
        current_state = self_->state;
      }
      switch (current_state) {
        case STARTING:
          // The compare exchange shouldn't fail with expected being STARTING
          heph::panic("Invalid State");
          return;
        case STARTED:
          // The operation was started, we can safely transition to the stopped case and carry on
          return;
        case COMPLETED:
          self_->onCompleted();
          return;
        case STOPPED:
          // The operation was marked as stopped, we should now call stop.
          self_->onStop();
          return;
        case ERROR:
          // If an error appeared, we should report it.
          self_->onError();
          return;
      }
    }

  private:
    StoppableOperationState* self_;
  };
  struct OnStopCallback {
    void operator()() const noexcept {
      self->stopRequested();
    }
    StoppableOperationState* self;
  };
  using EnvT = stdexec::env_of_t<Receiver>;
  using StopTokenT = stdexec::stop_token_of_t<EnvT>;
  using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, OnStopCallback>;

  [[nodiscard]] auto start() -> ToStartedTransition {
    on_stop_callback.emplace(stdexec::get_stop_token(stdexec::get_env(receiver)), OnStopCallback{ this });
    return ToStartedTransition(this);
  }

  [[nodiscard]] auto restart() -> std::optional<ToStartedTransition> {
    State previous_state{ STARTED };
    {
      absl::MutexLock l{ &mutex };
      previous_state = std::exchange(state, STARTING);
    }
    switch (previous_state) {
      case STARTED:
        // The restart operation should only succeed if the previous state was
        // STARTED
        break;
      // restart might get called concurrently to any  other transition. Just ignore in that case
      case STARTING:
      case STOPPED:
        return std::nullopt;
      case COMPLETED:
        heph::panic("restart called concurrently with setValue");
        break;
      case ERROR:
        heph::panic("restart called concurrently with setError");
        break;
    }
    return ToStartedTransition(this);
  }

  void stopRequested() {
    State previous_state{ STARTED };
    {
      absl::MutexLock l{ &mutex };
      previous_state = std::exchange(state, STOPPED);
    }
    switch (previous_state) {
      case STARTING:
        // In the case we were still in starting, there is nothing more todo, the ToStartedTransition will set
        // stopped eventually.
        return;
      case STARTED:
        // The operation was started, we can safely transition to the stopped case and carry on
        onStop();
        return;
      case COMPLETED:
        // This shouldn't happen. The callback should have been reset before switching to the completed state.
        heph::panic("stopRequested observed completed state");
        return;
      case STOPPED:
        // This really shouldn't happen. We are the only ones supposed to call stopped...
        heph::panic("Stopped called multiple times");
        return;
      case ERROR:
        // If an error appeared, we should report it.
        onError();
        return;
    }
  }

  template <typename... Ts>
  void setValue(Ts&&... ts) {
    State previous_state{ STARTED };
    {
      absl::MutexLock l{ &mutex };
      on_stop_callback.reset();
      values.emplace(std::forward<Ts>(ts)...);
      previous_state = std::exchange(state, COMPLETED);
    }
    switch (previous_state) {
      case STARTING:
        // In the case we were still in starting, there is nothing more todo, the ToStartedTransition will set
        // the values eventually.
        return;
      case STOPPED:
        // When STOPPED was triggered before, we should override this signal by properly
        // completing
      case STARTED:
        // The operation was started, we can safely transition to the completed stage
        onCompleted();
        return;
      case COMPLETED:
        // This really shouldn't happen. We are the only ones supposed to call setValue...
        heph::panic("Completed called multiple times");
        return;
      case ERROR:
        // If an error appeared, we should report it.
        onError();
        return;
    }
  }

  void setError(std::exception_ptr exception) {
    State previous_state{ STARTED };
    {
      absl::MutexLock l{ &mutex };
      on_stop_callback.reset();
      error = std::move(exception);
      previous_state = std::exchange(state, ERROR);
    }
    switch (previous_state) {
      case STARTING:
        // In the case we were still in starting, there is nothing more todo, the ToStartedTransition will set
        // the error eventually.
        return;
      case STARTED:
        // The operation was started, we can safely transition to the error stage
        onError();
        return;
      case COMPLETED:
        // This really shouldn't happen. We are the only ones supposed to call setValue...
        heph::panic("setError observed completed state");
        return;
      case STOPPED:
        // At this stage, we shouldn't be able to get stopped anymore...
        heph::panic("setError observed stopped state");
        return;
      case ERROR:
        // If an error appeared, we should report it.
        heph::panic("setError called multiple times");
        return;
    }
  }

  void onCompleted() {
    std::apply([this]<typename... Ts>(
                   Ts&&... ts) { stdexec::set_value(std::move(receiver), std::forward<Ts>(ts)...); },
               std::move(*values));
  }
  void onStop() {
    on_stop();
    stdexec::set_stopped(std::move(receiver));
  }
  void onError() {
    stdexec::set_error(std::move(receiver), error);
  }

  Receiver receiver;
  heph::UniqueFunction<void()> on_stop;
  absl::Mutex mutex;
  State state{ STARTING };
  std::optional<StopCallbackT> on_stop_callback;
  std::optional<std::tuple<Values...>> values;
  std::exception_ptr error;
};
}  // namespace heph::concurrency
