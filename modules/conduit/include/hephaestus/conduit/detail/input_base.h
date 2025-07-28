//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <fmt/format.h>
#include <hephaestus/conduit/detail/node_base.h>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__sender_introspection.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/basic_sender.h"
#include "hephaestus/conduit/detail/awaiter.h"
#include "hephaestus/conduit/detail/circular_buffer.h"
#include "hephaestus/conduit/input.h"
#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_handle.h"
#include "hephaestus/conduit/output.h"
#include "hephaestus/containers/intrusive_fifo_queue.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/utils.h"

namespace heph::conduit::detail {
struct InputPollT {};
struct InputBlockT {};

template <typename T>
struct IsOptional : std::false_type {};
template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template <typename T>
static constexpr bool ISOPTIONAL = IsOptional<std::decay_t<T>>::value;

template <typename InputT, typename T, std::size_t Depth>
class InputBase {
public:
  using DataT = T;

  InputBase(NodeBase* node, std::string name) : node_{ node }, name_(std::move(name)) {
    node_->addInputSpec([this] {
      return InputSpecification{
        .name = name_,
        .node_name = node_->nodeName(),
        .type = heph::utils::getTypeName<DataT>(),
      };
    });
  }

  auto name() {
    return fmt::format("{}/{}", node_->nodeName(), name_);
  }

  auto get()
    requires(InputT::InputPolicyT::RETRIEVAL_METHOD == RetrievalMethod::POLL)
  {
    return heph::concurrency::makeSenderExpression<detail::InputPollT>(static_cast<InputT*>(this));
  }

  auto get()
    requires(InputT::InputPolicyT::RETRIEVAL_METHOD == RetrievalMethod::BLOCK)
  {
    return heph::concurrency::makeSenderExpression<detail::InputBlockT>(static_cast<InputT*>(this));
  }

  auto node() -> NodeBase* {
    return node_;
  }

  template <typename OperationT, typename DataT>
  void connectTo(Node<OperationT, DataT>& node) {
    using ValueT = typename InputT::ValueT;
    using ExecuteOperationT = decltype(node.executeSender());
    using ExecuteResultVariantT = stdexec::value_types_of_t<ExecuteOperationT>;
    static_assert(std::variant_size_v<ExecuteResultVariantT> == 1);
    using ExecuteResultTupleT = std::variant_alternative_t<0, ExecuteResultVariantT>;
    static_assert(std::tuple_size_v<ExecuteResultTupleT> != 0, "Attempt to connect to node output of void");
    static_assert(std::tuple_size_v<ExecuteResultTupleT> == 1,
                  "Node returning more than one output should be impossible");
    using ResultT = std::tuple_element_t<0, ExecuteResultTupleT>;
    if constexpr (detail::ISOPTIONAL<ResultT>) {
      static_assert(std::is_same_v<ValueT, typename std::decay_t<ResultT>::value_type>,
                    "Input and output types don't match");
    } else {
      static_assert(std::is_same_v<ValueT, ResultT>, "Input and output types don't match");
    }
    node.registerInput(static_cast<InputT*>(this));
  }

  template <typename U>
  void connectTo(Output<U>& output) {
    static_assert(std::is_same_v<U, T>, "Input and output types don't match");
    output.registerInput(static_cast<InputT*>(this));
  }

  template <typename OperationT>
  void connectTo(NodeHandle<OperationT>& node) {
    connectTo(*node);
  }

  template <typename U>
  auto setValue(U&& u) -> InputState {
    if (!node_->runsOnEngine()) {
      // Dispatch to the engine scheduler to avoid race conditions
      auto res =
          stdexec::sync_wait(node_->scheduler().schedule() | stdexec::then([this, u = std::forward<U>(u)] {
                               return setValue(std::move(u));
                             }));
      if (!res.has_value()) {
        panic("Could not set value, engine was stopped");
      }
      return std::get<0>(*res);
    }
    auto push_result = buffer_.push(std::forward<U>(u));
    if (!push_result) {
      if (InputT::InputPolicyT::SET_METHOD == SetMethod::BLOCK) {
        return InputState::OVERFLOW;
      }
      buffer_.pop();
      return setValue(std::forward<U>(u));
    }
    this->triggerAwaiter();
    return InputState::OK;
  }

private:
  void enqueueWaiter(detail::AwaiterBase* awaiter) {
    if (containers::IntrusiveFifoQueueAccess::next(awaiter) == nullptr) {
      awaiters_.enqueue(awaiter);
    }
  }
  void dequeueWaiter(detail::AwaiterBase* awaiter) {
    awaiters_.erase(awaiter);
  }

protected:
  void triggerAwaiter() {
    auto* awaiter = awaiters_.dequeue();
    if (awaiter != nullptr) {
      awaiter->trigger();
    }
  }

protected:
  // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  detail::CircularBuffer<T, Depth> buffer_;
  // NOLINTEND(misc-non-private-member-variables-in-classes)
  // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

private:
  template <typename OtherInputT, typename ReceiverT>
  friend class Awaiter;

  containers::IntrusiveFifoQueue<AwaiterBase> awaiters_;
  std::string name_;
  NodeBase* node_;
};
}  // namespace heph::conduit::detail

// Sender implementation for polling input retrieval
namespace heph::concurrency {
template <>
struct SenderExpressionImpl<heph::conduit::detail::InputPollT> : DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = []<typename Sender>(Sender&&, Ignore = {}) noexcept {
    using InputT = std::remove_pointer_t<stdexec::__data_of<Sender>>;
    using ValueT = decltype(std::declval<InputT&>().getValue());

    return stdexec::completion_signatures<
        stdexec::set_value_t(ValueT), stdexec::set_error_t(std::exception_ptr), stdexec::set_stopped_t()>{};
  };

  static constexpr auto START = []<typename Receiver>(auto* self, Receiver&& receiver) {
    auto stop_token = stdexec::get_stop_token(stdexec::get_env(receiver));
    if (stop_token.stop_requested()) {
      stdexec::set_stopped(std::forward<Receiver>(receiver));
      return;
    }
    stdexec::set_value(std::forward<Receiver>(receiver), self->getValue());
  };
};
}  // namespace heph::concurrency

// Sender implementation for blocking input retrieval
namespace heph::concurrency {
template <>
struct SenderExpressionImpl<heph::conduit::detail::InputBlockT> : DefaultSenderExpressionImpl {
  static constexpr auto GET_COMPLETION_SIGNATURES = []<typename Sender>(Sender&&, Ignore = {}) noexcept {
    using InputT = std::remove_pointer_t<stdexec::__data_of<Sender>>;
    using ValueT = decltype(std::declval<InputT&>().getValue())::value_type;

    return stdexec::completion_signatures<
        stdexec::set_value_t(ValueT), stdexec::set_error_t(std::exception_ptr), stdexec::set_stopped_t()>{};
  };

  static constexpr auto GET_STATE = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver&& receiver) {
    using InputT = std::remove_pointer_t<stdexec::__data_of<Sender>>;
    auto [_, self] = std::forward<Sender>(sender);
    return typename InputT::template Awaiter<Receiver>{ self, std::forward<Receiver>(receiver) };
  };

  static constexpr auto START = [](auto& awaiter, Ignore) { awaiter.trigger(); };
};
}  // namespace heph::concurrency
