//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <optional>

#include <stdexec/execution.hpp>

namespace heph::conduit::internal {
// Helper for inputs which should never trigger
struct NeverStop {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(bool), stdexec::set_stopped_t()>;
  template <typename Receiver>
  struct Operation {
    struct StopCallback {
      void operator()() const noexcept {
        self->stop_callback.reset();
        stdexec::set_stopped(std::move(self->receiver));
      }
      Operation* self;
    };
    using ReceiverEnvT = stdexec::env_of_t<Receiver>;
    using StopTokenT = stdexec::stop_token_of_t<ReceiverEnvT>;
    using StopCallbackT = stdexec::stop_callback_for_t<StopTokenT, StopCallback>;
    Receiver receiver;
    std::optional<StopCallbackT> stop_callback;
    void start() & noexcept {
      stop_callback.emplace(stdexec::get_stop_token(stdexec::get_env(receiver)), StopCallback{ this });
    }
  };

  template <stdexec::receiver_of<completion_signatures> Receiver>
  auto connect(Receiver&& receiver) {
    return Operation<std::decay_t<Receiver>>{ std::forward<Receiver>(receiver) };
  }
};
}  // namespace heph::conduit::internal
