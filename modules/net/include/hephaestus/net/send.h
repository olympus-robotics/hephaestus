//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/net/detail/operation_state.h"
#include "hephaestus/net/socket.h"

namespace heph::net {
template <bool SendAll>
struct SendT {
  auto operator()(const Socket& socket, std::span<const std::byte> buffer) const {
    return concurrency::makeSenderExpression<SendT>(std::tuple{ &socket, buffer });
  }
};

// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr SendT<false> send{};
// NOLINTNEXTLINE(readability-identifier-naming)
inline constexpr SendT<true> sendAll{};

namespace internal {
template <bool SendAll, typename Receiver>
struct SendOperation {
  using StopTokenT = stdexec::stop_token_of_t<stdexec::env_of_t<Receiver>>;

  const Socket* socket{ nullptr };
  std::span<const std::byte> buffer;
  Receiver receiver;
  std::size_t transferred{ 0 };

  void prepare(::io_uring_sqe* sqe) const {
    auto send_size = std::min(socket->maximumSendSize(), buffer.size() - transferred);
    auto to_transfer = buffer.subspan(transferred, send_size);
    ::io_uring_prep_send(sqe, socket->nativeHandle(), to_transfer.data(), to_transfer.size(), MSG_NOSIGNAL);
  }

  auto handleCompletion(::io_uring_cqe* cqe) -> bool {
    if (cqe->res < 0) {
      stdexec::set_error(std::move(receiver), std::error_code(-cqe->res, std::system_category()));
      return true;
    }
    transferred += static_cast<std::size_t>(cqe->res);
    if constexpr (SendAll) {
      if (transferred != buffer.size()) {
        return false;
      }
    }
    stdexec::set_value(std::move(receiver), buffer.subspan(0, transferred));
    return true;
  }

  void handleStopped() {
    stdexec::set_stopped(std::move(receiver));
  }

  auto getStopToken() {
    return stdexec::get_stop_token(stdexec::get_env(receiver));
  }
};

template <bool SendAll>
struct SendSender : heph::concurrency::DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = []<typename Sender>(
                                                        Sender&&, heph::concurrency::Ignore = {}) noexcept {
    return stdexec::completion_signatures<stdexec::set_value_t(std::span<const std::byte>),
                                          stdexec::set_error_t(std::error_code), stdexec::set_stopped_t()>{};
  };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) noexcept {
    auto [_, data] = std::forward<Sender>(sender);
    auto [socket, buffer] = data;
    auto* ring = socket->context().ring();
    using SendOperationT = SendOperation<SendAll, std::decay_t<Receiver>>;
    using OperationStateT = detail::OperationState<SendOperationT>;
    return OperationStateT{ ring, SendOperationT{ socket, buffer, std::forward<Receiver>(receiver), 0 } };
  };

  static constexpr auto START = [](auto& operation, heph::concurrency::Ignore) { operation.submit(); };
};
}  // namespace internal
}  // namespace heph::net

namespace heph::concurrency {
template <bool sendAll>
struct SenderExpressionImpl<heph::net::SendT<sendAll>> : heph::net::internal::SendSender<sendAll> {};
}  // namespace heph::concurrency
