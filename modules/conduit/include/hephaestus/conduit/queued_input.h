
#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/node_operation_handle.h"

namespace heph::conduit {

template <typename Operation, typename OutputType = void>
struct NodeOperation;

enum class InputState : std::uint8_t {
  OK,
  OVERFLOW,
};

struct QueuedInputJustT {};
struct QueuedInputAwaitT {};

template <typename T, std::size_t QueueDepth = 1>
struct QueuedInput {
  explicit QueuedInput(std::nullptr_t, std::string_view name) : name(name) {
  }

  template <typename Operation, typename OutputType>
  explicit QueuedInput(NodeOperation<Operation, OutputType>* n, std::string_view name)
    : node{ n }, name(name) {
    n->registerInput(this);
  }
  template <typename Operation, typename OutputType, typename U>
  explicit QueuedInput(NodeOperation<Operation, OutputType>* node, std::string_view name, U&& initial)
    : QueuedInput(node, name) {
    setValue(std::forward<U>(initial));
  }

  auto await() {
    return stdexec::__make_sexpr<QueuedInputAwaitT>(this);
  }

  auto just() {
    return stdexec::__make_sexpr<QueuedInputJustT>(this);
  }

  template <typename U>
  auto setValue(U&& u) -> InputState {
    if (size == QueueDepth) {
      return InputState::OVERFLOW;
    }
    auto index = (current + size) % QueueDepth;
    data.at(index) = std::forward<U>(u);
    ++size;
    if (!awaiters.empty()) {
      auto* awaiter = awaiters.front();
      awaiters.pop_front();
      awaiter->trigger();
    }

    return InputState::OK;
  }

  auto getValue() -> std::optional<T> {
    if (size == 0) {
      return std::nullopt;
    }
    auto index = current;

    current = (current + 1) % QueueDepth;
    --size;
    return std::optional<T>{ std::move(data.at(index)) };
  }

  template <typename Operation, typename OutputType>
  void setParent(NodeOperation<Operation, OutputType>* p) {
    fmt::println("input {} set parent {}", name, p->getName());
    parent = p;
  }
  void setParent(NodeOperationHandle p) {
    fmt::println("input {} set parent handle {}", name, p.name());
    parent = p;
  }

  using type = T;

  std::array<T, QueueDepth> data{};
  std::size_t size{ 0 };
  std::size_t current{ 0 };
  NodeOperationHandle node;
  std::string_view name;
  NodeOperationHandle parent;

  struct AwaiterBase {
    virtual ~AwaiterBase() = default;

    virtual void trigger() = 0;
  };

  std::deque<AwaiterBase*> awaiters;

  void enqueueWaiter(AwaiterBase* awaiter) {
    awaiters.push_back(awaiter);
  }

  template <typename Receiver>
  struct Awaiter : AwaiterBase {
    QueuedInput* self;
    Receiver receiver;

    Awaiter(QueuedInput* self, Receiver& receiver) : self(self), receiver(receiver) {
    }

    void start() {
    }

    void trigger() override {
      auto res = self->getValue();
      if (res.has_value()) {
        stdexec::set_value(std::move(receiver), std::move(*res));
        return;
      }

      self->enqueueWaiter(this);
    }
  };
};
}  // namespace heph::conduit

// Implementation for QueuedInput
namespace stdexec {
template <>
struct __sexpr_impl<heph::conduit::QueuedInputJustT> : __sexpr_defaults {
  // NOLINTBEGIN
  static constexpr auto get_completion_signatures = []<typename Sender>(Sender&& /*sender*/, auto&&...) {
    using type = typename std::remove_pointer_t<std::decay_t<stdexec::__data_of<Sender>>>::type;
    return stdexec::completion_signatures<stdexec::set_value_t(std::optional<type>),
                                          stdexec::set_stopped_t()>{};
  };

  static constexpr auto start = [](auto* self, auto& receiver) {
    stdexec::set_value(receiver, self->getValue());
  };
  // NOLINTEND
};

template <>
struct __sexpr_impl<heph::conduit::QueuedInputAwaitT> : __sexpr_defaults {
  // NOLINTBEGIN
  static constexpr auto get_completion_signatures = []<typename Sender>(Sender&& /*sender*/, auto&&...) {
    using type = typename std::remove_pointer_t<std::decay_t<stdexec::__data_of<Sender>>>::type;
    return stdexec::completion_signatures<stdexec::set_value_t(type), stdexec::set_stopped_t()>{};
  };

  static constexpr auto get_state = []<typename Sender, typename Receiver>(Sender&& sender,
                                                                           Receiver& receiver) {
    static_assert(stdexec::sender_expr_for<Sender, heph::conduit::QueuedInputAwaitT>);
    return sender.apply(std::forward<Sender>(sender), [&]<typename Input>(stdexec::__ignore, Input* self) {
      return typename Input::template Awaiter<Receiver>{ self, receiver };
    });
  };

  static constexpr auto start = [](auto& awaiter, stdexec::__ignore) { awaiter.trigger(); };
  // NOLINTEND
};

}  // namespace stdexec
