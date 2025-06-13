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
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/net/acceptor.h"
#include "hephaestus/net/detail/operation_state.h"
#include "hephaestus/net/socket.h"

namespace heph::net {
struct AcceptT {
  template <stdexec::sender Sender>
  auto operator()(Sender&& sender, Acceptor const& acceptor) const {
    auto domain = stdexec::__get_early_domain(sender);
    return stdexec::transform_sender(
        domain, concurrency::makeSenderExpression<AcceptT>(&acceptor, std::forward<Sender>(sender)));
  }
  template <stdexec::sender Sender>
  auto operator()(Sender&& sender, Acceptor const* acceptor) const {
    auto domain = stdexec::__get_early_domain(sender);
    return stdexec::transform_sender(
        domain, concurrency::makeSenderExpression<AcceptT>(acceptor, std::forward<Sender>(sender)));
  }

  auto operator()(Acceptor const& acceptor) const -> stdexec::__binder_back<AcceptT, Acceptor const*> {
    return { { &acceptor }, {}, {} };
  }
};

// NOLINTNEXTLINE (readability-identifier-naming)
static inline AcceptT accept{};

template <typename Receiver>
struct AcceptOperation {
  using StopTokenT = stdexec::stop_token_of_t<stdexec::env_of_t<Receiver>>;

  Acceptor const* acceptor{ nullptr };
  Receiver receiver;

  void prepare(::io_uring_sqe* sqe) const {
    ::io_uring_prep_accept(sqe, acceptor->nativeHandle(), nullptr, nullptr, 0);
  }
  void handleCompletion(::io_uring_cqe* cqe) {
    if (cqe->res < 0) {
      stdexec::set_error(std::move(receiver), std::error_code(-cqe->res, std::system_category()));
      return;
    }
    stdexec::set_value(std::move(receiver), Socket{ cqe->res });
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
  static constexpr auto GET_ATTRS =  //
      []<class Child>(heph::concurrency::Ignore, const Child& child) noexcept {
        return stdexec::__env::__join(
            stdexec::prop{ stdexec::__is_scheduler_affine_t{},
                           stdexec::__mbool<stdexec::__is_scheduler_affine<Child>>{} },
            stdexec::get_env(child));
      };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) noexcept {
    auto [_, acceptor, child] = std::forward<Sender>(sender);
    auto* ring =
        stdexec::get_completion_scheduler<stdexec::set_value_t>(stdexec::get_env(child)).context().ring();
    using AcceptOperationT = AcceptOperation<std::decay_t<Receiver>>;
    using OperationStateT = detail::OperationState<AcceptOperationT>;
    return OperationStateT{ ring, AcceptOperationT{ acceptor, std::forward<Receiver>(receiver) } };
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
struct SenderExpressionImpl<heph::net::AcceptT> : heph::net::AcceptSender {};
}  // namespace heph::concurrency
