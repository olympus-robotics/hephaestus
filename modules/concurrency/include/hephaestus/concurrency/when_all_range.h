//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

#include <boost/container/static_vector.hpp>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__meta.hpp>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/error_handling/panic.h"

namespace heph::concurrency {

template <typename Range>
concept SenderRange = std::ranges::range<Range> && stdexec::sender<std::ranges::range_value_t<Range>>;

namespace internal {

struct WhenAllStopCallback {
  stdexec::inplace_stop_source* stop_source;
  void operator()() const {
    stop_source->request_stop();
  }
};

enum class WhenAllRangeState : std::uint8_t { STARTED, ERROR, STOPPED };

template <typename Receiver>
struct WhenAllRangeStateT {
  using ReceiverEnv = stdexec::env_of_t<Receiver>;
  using StopToken = stdexec::stop_token_of_t<ReceiverEnv>;
  using StopCallbackT = stdexec::stop_callback_for_t<StopToken, WhenAllStopCallback>;

  void arrive() noexcept {
    if (1 == count.fetch_sub(1, std::memory_order_acq_rel)) {
      complete();
    }
  }

  void complete() noexcept {
    on_stop.reset();

    switch (state.load(std::memory_order_acquire)) {
      case WhenAllRangeState::STARTED:
        stdexec::set_value(std::move(receiver));
        break;
      case WhenAllRangeState::ERROR:
        stdexec::set_error(std::move(receiver), std::move(error));
        break;
      case WhenAllRangeState::STOPPED:
        stdexec::set_stopped(std::move(receiver));
    }
  }

  [[nodiscard]] auto getEnv() const noexcept {
    return stdexec::__env::__join(stdexec::prop{ stdexec::get_stop_token, stop_source.get_token() },
                                  stdexec::get_env(receiver));
  }

  Receiver receiver;
  std::atomic<std::size_t> count{ 0 };
  stdexec::inplace_stop_source stop_source;
  std::atomic<WhenAllRangeState> state{ WhenAllRangeState::STARTED };
  std::exception_ptr error;
  std::optional<StopCallbackT> on_stop;
};

template <typename OuterReceiver>
struct InnerReceiver {
  using receiver_concept = stdexec::receiver_t;

  // NOLINTBEGIN(readability-identifier-naming) - wrapping stdexec interface
  template <typename... Ts>
  void set_value(Ts&&... /*ts*/) noexcept {
    static_assert(sizeof...(Ts) == 0, "Only void senders supported at the moment");
    state->arrive();
  }

  void set_stopped() noexcept {
    WhenAllRangeState expected = WhenAllRangeState::STARTED;
    // Transition to the "stopped" state if and only if we're in the
    // "started" state. (If this fails, it's because we're in an
    // error state, which trumps cancellation.)
    if (state->state.compare_exchange_strong(expected, WhenAllRangeState::STOPPED,
                                             std::memory_order_acq_rel)) {
      state->stop_source.request_stop();
    }
    state->arrive();
  }

  void set_error(std::exception_ptr ptr) noexcept {
    switch (state->state.exchange(WhenAllRangeState::ERROR, std::memory_order_acq_rel)) {
      case WhenAllRangeState::STARTED:
        state->stop_source.request_stop();
        [[fallthrough]];
      case WhenAllRangeState::STOPPED:
        // We are the first child to complete with an error, so we must save the error. (Any
        // subsequent errors are ignored.)
        state->error = std::move(ptr);
        break;
      case WhenAllRangeState::ERROR:
          // We're already in the "error" state. Ignore the error.
          ;
    }
    state->arrive();
  }

  [[nodiscard]] auto get_env() const noexcept {
    return state->getEnv();
  }
  // NOLINTEND(readability-identifier-naming)
  WhenAllRangeStateT<OuterReceiver>* state;
};

template <std::size_t N, typename Range, typename Receiver>
struct Operation {
  using SenderT = std::ranges::range_value_t<Range>;
  using OuterReceiverEnv = stdexec::env_of_t<Receiver>;
  using InnerReceiverT = InnerReceiver<Receiver>;
  using InnerOperationT = stdexec::connect_result_t<SenderT, InnerReceiverT>;

  void start() noexcept {
    state.count.store(N, std::memory_order_release);
    state.on_stop.emplace(stdexec::get_stop_token(stdexec::get_env(state.receiver)),
                          WhenAllStopCallback{ &state.stop_source });
    for (auto& sender : range) {
      // NOTE: This might throw an exception (for example std::bad_alloc), as start is noexcept,
      // we are opting to have the program abort since this any exception thrown here is likely
      // a fatal bug.
      operations.emplace_back(stdexec::__emplace_from{
          [&]() { return stdexec::connect(std::move(sender), InnerReceiverT{ &state }); } });
      stdexec::start(operations.back());
    }
  }

  WhenAllRangeStateT<Receiver> state;
  Range range;
  boost::container::static_vector<InnerOperationT, N> operations;
};

template <std::size_t N, typename Range>
struct WhenAllRangeSender {
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_stopped_t(),
                                     stdexec::set_error_t(std::exception_ptr)>;

  template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
  auto connect(Receiver&& receiver) && -> Operation<N, Range, ReceiverT> {
    return { { std::forward<Receiver>(receiver) }, std::move(range) };
  }

  Range range;
};
}  // namespace internal

/// Wait on a range of senders with a fixed size.
///
/// \tparam N Number of elements in the range
/// \param range the range of senders to wait on. Takes ownership of the range
///
/// \note Currently only senders completing with void are supported
///
/// This function operates on a fixed size range. The
template <std::size_t N, SenderRange Range, typename RangeT = std::decay_t<Range>>
  requires(N > 0)
[[nodiscard]] auto whenAllRange(Range&& range) {
  HEPH_PANIC_IF(N != std::ranges::size(range), "Size mismatch");
  return internal::WhenAllRangeSender<N, RangeT>{ std::forward<Range>(range) };
}
}  // namespace heph::concurrency
