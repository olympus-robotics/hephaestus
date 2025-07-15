//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>
#include <sys/socket.h>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/net/detail/operation_state.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/net/socket.h"

namespace heph::net {
template <typename TagT>
struct ConnectT {
  auto operator()(const Socket& socket, const Endpoint& endpoint) const {
    return concurrency::makeSenderExpression<ConnectT>(std::tuple{ &socket, &endpoint });
  }
};

// NOLINTNEXTLINE (readability-identifier-naming)
inline constexpr ConnectT<void> connect{};

namespace internal {
template <typename Receiver>
struct ConnectOperation {
  using StopTokenT = stdexec::stop_token_of_t<stdexec::env_of_t<Receiver>>;

  const Socket* socket{ nullptr };
  const Endpoint* endpoint{ nullptr };
  Receiver receiver;

  void prepare(::io_uring_sqe* sqe) const {
    auto addr = endpoint->nativeHandle();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ::io_uring_prep_connect(sqe, socket->nativeHandle(), reinterpret_cast<const sockaddr*>(addr.data()),
                            static_cast<socklen_t>(addr.size()));
  }
  void handleCompletion(::io_uring_cqe* cqe) {
    if (cqe->res < 0) {
      stdexec::set_error(std::move(receiver), std::error_code(-cqe->res, std::system_category()));
      return;
    }
    stdexec::set_value(std::move(receiver));
  }

  void handleStopped() {
    stdexec::set_stopped(std::move(receiver));
  }

  auto getStopToken() {
    return stdexec::get_stop_token(stdexec::get_env(receiver));
  }
};

struct ConnectSender : heph::concurrency::DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES =
      []<typename Sender>(Sender&&, heph::concurrency::Ignore = {}) noexcept {
        return stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::error_code),
                                              stdexec::set_stopped_t()>{};
      };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) noexcept {
    auto [_, data] = std::forward<Sender>(sender);
    auto [socket, endpoint] = data;
    auto* ring = socket->context().ring();
    using ConnectOperationT = ConnectOperation<std::decay_t<Receiver>>;
    using OperationStateT = detail::OperationState<ConnectOperationT>;
    return OperationStateT{ ring, ConnectOperationT{ socket, std::move(endpoint),
                                                     std::forward<Receiver>(receiver) } };
  };

  static constexpr auto START = [](auto& operation, heph::concurrency::Ignore) { operation.submit(); };
};
}  // namespace internal
}  // namespace heph::net

namespace heph::concurrency {
template <>
struct SenderExpressionImpl<heph::net::ConnectT<void>> : heph::net::internal::ConnectSender {};
}  // namespace heph::concurrency
