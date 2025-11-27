//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <system_error>
#include <type_traits>
#include <utility>

#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__optional.hpp>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

namespace heph::concurrency {

namespace internal {
template <typename SenderFactoryT>
concept SenderFactory = requires(SenderFactoryT sender_factory) {
  { sender_factory() } -> stdexec::sender;
};

template <typename SenderFactory>
struct RepeatUntilSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t(),
                                     stdexec::set_error_t(std::exception_ptr)>;

  SenderFactory sender_factory;

  using SenderT = std::invoke_result_t<SenderFactory>;

  template <typename Receiver>
  struct Operation {
    struct InnerReceiver {
      using receiver_concept = stdexec::receiver_t;
      Operation* self;

      // NOLINTBEGIN(readability-identifier-naming) - wrapping stdexec interface
      void set_value(bool done) const noexcept {
        self->next(done);
      }

      void set_stopped() const noexcept {
        self->setStopped();
      }

      template <typename Error>
      void set_error(Error error) const noexcept {
        self->setError(std::move(error));
      }

      auto get_env() const noexcept {
        return stdexec::get_env(self->receiver);
      }
      // NOLINTEND(readability-identifier-naming)
    };

    SenderFactory sender_factory;
    Receiver receiver;
    stdexec::__optional<stdexec::connect_result_t<SenderT, InnerReceiver>> state;

    void start() noexcept {
      next(false);
    }

    void next(bool done) {
      if (done) {
        stdexec::set_value(std::move(receiver));
        return;
      }
      try {
        state.emplace(stdexec::__emplace_from{
            [this]() { return stdexec::connect(sender_factory(), InnerReceiver{ this }); } });

      } catch (...) {
        stdexec::set_error(std::move(receiver), std::current_exception());
        return;
      }
      stdexec::start(*state);
    }

    void setStopped() {
      stdexec::set_stopped(std::move(receiver));
    }

    template <typename Error>
    void setError(Error error) noexcept {
      if constexpr (std::is_same_v<std::decay_t<Error>, std::exception_ptr>) {
        stdexec::set_error(std::move(receiver), std::move(error));
      }
      if constexpr (std::is_same_v<std::decay_t<Error>, std::error_code>) {
        stdexec::set_error(std::move(receiver), std::make_exception_ptr(std::system_error(std::move(error))));
      } else {
        stdexec::set_error(std::move(receiver), std::make_exception_ptr(std::move(error)));
      }
    }
  };

  template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
  auto connect(Receiver&& receiver) -> Operation<ReceiverT> {
    return Operation<ReceiverT>{ std::move(sender_factory), std::forward<Receiver>(receiver) };
  }
};
}  // namespace internal

/// repeatedly starts \param sender_factory until the completion of the sender returns `false`.
///
/// \param sender_factory nullary function object returning a sender with a bool completion set_value
/// completion.
///
/// This can be used to implement loops using sender receivers. By passing a factory instead of the sender
/// directly, we are able to use movable only senders.
///
/// The return sender has the following completion signatures:
///  - `set_value()`: repeatUntil completes once the passed sender completes with true
///  - `set_error(std::exception_ptr)`: if an exception occurs, it will be forwarded. `std::error_code` will
///  be wrapped inside `std::system_error`. Every other error will be turned into an exception_ptr via
///  `std::make_exception_ptr`.
///  - `set_stopped()`
template <internal::SenderFactory SenderFactoryT>
auto repeatUntil(SenderFactoryT&& sender_factory) {
  return internal::RepeatUntilSender<std::decay_t<SenderFactoryT>>{ std::forward<SenderFactoryT>(
      sender_factory) };
}

/// Convenience wrapper for \ref repeatUntil for cases where \param sender is copyable
template <stdexec::sender SenderT>
auto repeatUntil(SenderT&& sender) {
  return repeatUntil([sender = std::forward<SenderT>(sender)] { return sender; });
}
}  // namespace heph::concurrency
