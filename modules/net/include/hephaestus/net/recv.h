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
struct RecvT {
  auto operator()(const Socket& socket, std::span<std::byte> buffer) const {
    return concurrency::makeSenderExpression<RecvT>(std::tuple{ &socket, buffer });
  }
};

// NOLINTNEXTLINE (readability-identifier-naming)
inline constexpr RecvT<false> recv{};
// NOLINTNEXTLINE (readability-identifier-naming)
inline constexpr RecvT<true> recvAll{};

namespace internal {
template <bool RecvAll, typename Receiver>
struct RecvOperation {
  using StopTokenT = stdexec::stop_token_of_t<stdexec::env_of_t<Receiver>>;

  const Socket* socket{ nullptr };
  std::span<std::byte> buffer;
  Receiver receiver;
  std::size_t transferred{ 0 };

  void prepare(::io_uring_sqe* sqe) const {
    auto recv_size = std::min(socket->maximumRecvSize(), buffer.size() - transferred);
    auto to_transfer = buffer.subspan(transferred, recv_size);
    ::io_uring_prep_recv(sqe, socket->nativeHandle(), to_transfer.data(), to_transfer.size(), 0);
  }

  auto handleCompletion(::io_uring_cqe* cqe) -> bool {
    if (cqe->res < 0) {
      stdexec::set_error(std::move(receiver), std::error_code(-cqe->res, std::system_category()));
      return true;
    }
    if (cqe->res == 0) {
      stdexec::set_stopped(std::move(receiver));
      return true;
    }
    transferred += cqe->res;
    if constexpr (RecvAll) {
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

template <bool RecvAll>
struct RecvSender : heph::concurrency::DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = []<typename Sender>(
                                                        Sender&&, heph::concurrency::Ignore = {}) noexcept {
    return stdexec::completion_signatures<stdexec::set_value_t(std::span<std::byte>),
                                          stdexec::set_error_t(std::error_code), stdexec::set_stopped_t()>{};
  };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) noexcept {
    auto [_, data] = std::forward<Sender>(sender);
    auto [socket, buffer] = data;
    auto* ring = socket->context().ring();
    using RecvOperationT = RecvOperation<RecvAll, std::decay_t<Receiver>>;
    using OperationStateT = detail::OperationState<RecvOperationT>;
    return OperationStateT{ ring, RecvOperationT{ socket, buffer, std::forward<Receiver>(receiver), 0 } };
  };

  static constexpr auto START = [](auto& operation, heph::concurrency::Ignore) { operation.submit(); };
};
}  // namespace internal
}  // namespace heph::net

namespace heph::concurrency {
template <bool RecvAll>
struct SenderExpressionImpl<heph::net::RecvT<RecvAll>> : heph::net::internal::RecvSender<RecvAll> {};
}  // namespace heph::concurrency
