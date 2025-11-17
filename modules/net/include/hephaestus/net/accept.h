//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <system_error>
#include <type_traits>
#include <utility>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/detail/operation_state.h"
#include "hephaestus/net/socket.h"

namespace heph::net {
template <typename TagT>
struct AcceptT {
  auto operator()(const Acceptor& acceptor) const {
    return concurrency::makeSenderExpression<AcceptT>(&acceptor);
  }
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr AcceptT<void> accept{};

namespace internal {
template <typename Receiver>
struct AcceptOperation {
  using StopTokenT = stdexec::stop_token_of_t<stdexec::env_of_t<Receiver>>;

  const Acceptor* acceptor{ nullptr };
  // absl::Mutex mutex;
  Receiver receiver;

  void prepare(::io_uring_sqe* sqe) const {
    ::io_uring_prep_accept(sqe, acceptor->nativeHandle(), nullptr, nullptr, 0);
  }
  void handleCompletion(::io_uring_cqe* cqe) {
    if (cqe->res < 0) {
      stdexec::set_error(std::move(receiver), std::error_code(-cqe->res, std::system_category()));
      return;
    }
    stdexec::set_value(std::move(receiver),
                       Socket{ &acceptor->context(), cqe->res, acceptor->type(), false });
  }

  void handleStopped() {
    stdexec::set_stopped(std::move(receiver));
  }

  auto getStopToken() {
    return stdexec::get_stop_token(stdexec::get_env(receiver));
  }
};

struct AcceptSender : heph::concurrency::DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = []<typename Sender>(
                                                        Sender&&, heph::concurrency::Ignore = {}) noexcept {
    return stdexec::completion_signatures<stdexec::set_value_t(Socket), stdexec::set_error_t(std::error_code),
                                          stdexec::set_stopped_t()>{};
  };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) noexcept {
    auto [_, acceptor] = std::forward<Sender>(sender);
    auto* ring = acceptor->context().ring();
    using AcceptOperationT = AcceptOperation<std::decay_t<Receiver>>;
    using OperationStateT = detail::OperationState<AcceptOperationT>;
    return OperationStateT{ ring, AcceptOperationT{ acceptor, std::forward<Receiver>(receiver) } };
  };

  static constexpr auto START = [](auto& operation, heph::concurrency::Ignore) { operation.submit(); };
};
}  // namespace internal
}  // namespace heph::net

namespace heph::concurrency {
template <>
struct SenderExpressionImpl<heph::net::AcceptT<void>> : heph::net::internal::AcceptSender {};
}  // namespace heph::concurrency
