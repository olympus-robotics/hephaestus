//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <type_traits>
#include <utility>

#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__optional.hpp>
#include <stdexec/__detail/__sender_introspection.hpp>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"

namespace heph::concurrency {

namespace internal {
template <typename SenderFactoryT>
concept SenderFactory = requires(SenderFactoryT sender_factory) {
  { sender_factory() } -> stdexec::sender;
};
}  // namespace internal

struct RepeatUntilT {
  template <stdexec::sender SenderT>
  auto operator()(SenderT&& sender) const {
    return (*this)([sender = std::forward<SenderT>(sender)] { return sender; });
  }

  template <internal::SenderFactory SenderFactoryT>
  auto operator()(SenderFactoryT&& sender_factory) const {
    return concurrency::makeSenderExpression<RepeatUntilT>(std::forward<SenderFactoryT>(sender_factory));
  }
};

// NOLINTNEXTLINE (readability-identifier-naming)
inline constexpr RepeatUntilT repeatUntil{};

namespace internal {
template <typename SenderFactoryT, typename ReceiverT>
struct RepeatUntilStateT {
  using SenderT = std::invoke_result_t<SenderFactoryT>;
  struct InnerReceiverT {
    RepeatUntilStateT* self;
    using receiver_concept = stdexec::receiver_t;

    // NOLINTNEXTLINE (readability-identifier-naming)
    void set_value(bool done) const noexcept {
      if (done) {
        stdexec::set_value(std::move(self->reciever));
        return;
      }
      self->start();
    }

    // NOLINTNEXTLINE (readability-identifier-naming)
    void set_stopped() const noexcept {
      stdexec::set_stopped(std::move(self->reciever));
    }

    template <typename Error>
    // NOLINTNEXTLINE (readability-identifier-naming)
    void set_error(Error error) const noexcept {
      if constexpr (std::is_same_v<std::decay_t<Error>, std::exception_ptr>) {
        stdexec::set_error(std::move(self->reciever), std::move(error));
      } else {
        try {
          throw error;
        } catch (...) {
          stdexec::set_error(std::move(self->reciever), std::current_exception());
        }
      }
    }

    // NOLINTNEXTLINE (readability-identifier-naming)
    auto get_env() const noexcept {
      return stdexec::get_env(self->reciever);
    }
  };
  SenderFactoryT sender_factory;
  ReceiverT reciever;
  stdexec::__optional<stdexec::connect_result_t<SenderT, InnerReceiverT>> state;

  void start() {
    try {
      state.emplace(stdexec::__emplace_from{
          [this]() { return stdexec::connect(sender_factory(), InnerReceiverT{ this }); } });

    } catch (...) {
      stdexec::set_error(std::move(reciever), std::current_exception());
      return;
    }
    stdexec::start(*state);
  }
};

}  // namespace internal

template <>
struct SenderExpressionImpl<RepeatUntilT> : DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = []<typename Sender>(
                                                        Sender&&, heph::concurrency::Ignore = {}) noexcept {
    return stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr),
                                          stdexec::set_stopped_t()>{};
  };
  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) noexcept {
    auto [_, sender_factory] = std::forward<Sender>(sender);
    using SenderFactoryT = std::decay_t<stdexec::__data_of<Sender>>;
    using ReceivertT = std::decay_t<Receiver>;
    return internal::RepeatUntilStateT<SenderFactoryT, ReceivertT>{ std::move(sender_factory),
                                                                    std::forward<Receiver>(receiver) };
  };
  static constexpr auto START = []<typename Receiver>(auto& self, Receiver&& /*receiver*/) { self.start(); };
};

}  // namespace heph::concurrency
