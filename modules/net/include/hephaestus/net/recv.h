//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <span>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/net/detail/operation_state.h"
#include "hephaestus/net/socket.h"

namespace heph::net {
struct RecvT {
  template <stdexec::sender Sender>
  auto operator()(Sender&& sender, Socket const& socket, std::span<std::byte> buffer) const {
    auto domain = stdexec::__get_early_domain(sender);
    return stdexec::transform_sender(domain,
                                     concurrency::makeSenderExpression<RecvT>(std::tuple{ &socket, buffer },
                                                                              std::forward<Sender>(sender)));
  }
  template <stdexec::sender Sender>
  auto operator()(Sender&& sender, Socket const* socket, std::span<std::byte> buffer) const {
    auto domain = stdexec::__get_early_domain(sender);
    return stdexec::transform_sender(domain, concurrency::makeSenderExpression<RecvT>(
                                                 std::tuple{ socket, buffer }, std::forward<Sender>(sender)));
  }

  auto operator()(Socket const& socket, std::span<std::byte> buffer) const
      -> stdexec::__binder_back<RecvT, Socket const*, std::span<std::byte>> {
    return { { &socket, buffer }, {}, {} };
  }
};

// NOLINTNEXTLINE (readability-identifier-naming)
static inline RecvT recv{};

template <typename Receiver>
struct RecvOperation {
  using StopTokenT = stdexec::stop_token_of_t<stdexec::env_of_t<Receiver>>;

  Socket const* socket{ nullptr };
  std::span<std::byte> buffer;
  Receiver receiver;
  std::size_t transferred{ 0 };

  void prepare(::io_uring_sqe* sqe) const {
    ::io_uring_prep_recv(sqe, socket->nativeHandle(), buffer.data(), buffer.size(), 0);
  }
  void handleCompletion(::io_uring_cqe* cqe) {
    if (cqe->res < 0) {
      stdexec::set_error(std::move(receiver), std::error_code(-cqe->res, std::system_category()));
      return;
    }
    if (cqe->res == 0) {
      stdexec::set_stopped(std::move(receiver));
      return;
    }
    transferred += cqe->res;
    stdexec::set_value(std::move(receiver), buffer.subspan(0, transferred));
  }

  void handleStopped() const {
  }

  auto getStopToken() {
    return stdexec::get_stop_token(stdexec::get_env(receiver));
  }
};

struct RecvSender : heph::concurrency::DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = []<typename Sender>(
                                                        Sender&&, heph::concurrency::Ignore = {}) noexcept {
    return stdexec::completion_signatures<stdexec::set_value_t(std::span<std::byte>),
                                          stdexec::set_error_t(std::error_code), stdexec::set_stopped_t()>{};
  };
  static constexpr auto GET_ATTRS =  //
      []<class Child>(heph::concurrency::Ignore, const Child& child) noexcept {
        return stdexec::__env::__join(
            stdexec::prop{ stdexec::__is_scheduler_affine_t{},
                           stdexec::__mbool<stdexec::__is_scheduler_affine<Child>>{} },
            stdexec::get_env(child));
      };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) noexcept {
    auto [_, data, child] = std::forward<Sender>(sender);
    auto* ring =
        stdexec::get_completion_scheduler<stdexec::set_value_t>(stdexec::get_env(child)).context().ring();
    auto ring_stop_token = ring->getStopToken();
    auto env_stop_token = stdexec::get_stop_token(stdexec::get_env(receiver));
    auto [socket, buffer] = data;
    using RecvOperationT = RecvOperation<std::decay_t<Receiver>>;
    using OperationStateT = detail::OperationState<RecvOperationT>;
    return OperationStateT{ ring, RecvOperationT{ socket, buffer, std::forward<Receiver>(receiver), 0 } };
  };

  static constexpr auto COMPLETE = []<typename Receiver, typename Tag, typename... Args>(
                                       heph::concurrency::Ignore, auto& operation, Receiver& receiver,
                                       Tag /*tag*/, Args&&... args) noexcept {
    if constexpr (std::is_same_v<Tag, stdexec::set_value_t>) {
      operation.submit();
    } else {
      Tag()(std::move(receiver), std::forward<Args>(args)...);
    }
  };
};

}  // namespace heph::net

namespace heph::concurrency {
template <>
struct SenderExpressionImpl<heph::net::RecvT> : heph::net::RecvSender {};
}  // namespace heph::concurrency
