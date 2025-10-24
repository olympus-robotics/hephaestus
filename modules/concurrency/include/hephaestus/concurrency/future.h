//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>

#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"

namespace heph::concurrency {
template <typename T>
class Future {
  auto getEnv() -> stdexec::env<> {
    return stdexec::env<>{};
  }
  struct Receiver {
    using receiver_concept = stdexec::receiver_t;

    Future* self;

    template <typename... Ts>
    void set_value(Ts&&... /*ts*/) noexcept {
      self->done();
    }
    void set_stopped() noexcept {
      self->done();
    }
    void set_error(std::exception_ptr ptr) noexcept {
      (void)ptr;
      self->done();
    }

    [[nodiscard]] auto get_env() const noexcept {
      return self->getEnv();
    }
  };

  struct State {
    using Operation = stdexec::connect_result_t<AnySender<T>&&, Receiver>;

    std::atomic<bool> done{ false };
    Operation operation;
  };

public:
  explicit Future(AnySender<T> sender)
    : state_(new State{ { false }, stdexec::connect(std::move(sender), Receiver{ this }) }) {
    state_->operation.start();
  }

  Future(Future<T>&& other) = default;
  auto operator=(Future<T>&& other) -> Future& = default;
  Future(const Future<T>& other) = delete;
  auto operator=(const Future<T>& other) -> Future& = delete;

  auto get() {
    while (!state_->done.load(std::memory_order_acquire)) {
      state_->done.wait(false, std::memory_order_acquire);
    }
  }

private:
  void done() {
    state_->done.store(true, std::memory_order_release);
  }

private:
  // FIXME: memory leak!
  State* state_;
};
}  // namespace heph::concurrency
