//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::conduit::detail {
class AwaiterBase {
public:
  virtual ~AwaiterBase() = default;
  virtual void trigger() = 0;

private:
  friend struct containers::IntrusiveFifoQueueAccess;
  [[maybe_unused]] AwaiterBase* next_{ nullptr };
};

template <typename InputT, typename ReceiverT>
class Awaiter : public AwaiterBase {
  struct StopCallback {
    void operator()() const noexcept {
      self->handleStopped();
    }
    Awaiter* self;
  };
  using ReceiverEnvT = stdexec::env_of_t<ReceiverT>;
  using StopTokenT = stdexec::stop_token_of_t<ReceiverEnvT>;
  using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, StopCallback>;
  using ContextStopCallbackT = stdexec::stop_callback_for_t<stdexec::inplace_stop_token, StopCallback>;

public:
  Awaiter(InputT* self, ReceiverT receiver) : self_{ self }, receiver_{ std::move(receiver) } {
  }

  ~Awaiter() noexcept final {
    if (enqueued_) {
      self_->dequeueWaiter(this);
    }
  }

  Awaiter(Awaiter const&) = delete;
  Awaiter(Awaiter&&) = delete;
  auto operator=(Awaiter const&) -> Awaiter& = delete;
  auto operator=(Awaiter&&) -> Awaiter& = delete;

  void trigger() final {
    auto stop_token = stdexec::get_stop_token(stdexec::get_env(receiver_));
    auto context_stop_token = self_->node()->getStopToken();

    if (stop_token.stop_requested() || context_stop_token.stop_requested()) {
      stdexec::set_stopped(std::move(receiver_));
      return;
    }
    auto res = self_->getValue();
    if (res.has_value()) {
      stop_callback_.reset();
      context_stop_callback_.reset();
      stdexec::set_value(std::move(receiver_), std::move(*res));
      return;
    }

    if (!enqueued_) {
      enqueued_ = true;
      self_->enqueueWaiter(this);
    }
    if (!stop_callback_.has_value()) {
      stop_callback_.emplace(stop_token, StopCallback{ this });
      context_stop_callback_.emplace(context_stop_token, StopCallback{ this });
    }
  }

private:
  void handleStopped() {
    stop_callback_.reset();
    context_stop_callback_.reset();
    stdexec::set_stopped(std::move(receiver_));
  }

private:
  InputT* self_;
  ReceiverT receiver_;
  std::optional<StopCallbackT> stop_callback_;
  std::optional<ContextStopCallbackT> context_stop_callback_;
  bool enqueued_{ false };
};
}  // namespace heph::conduit::detail
