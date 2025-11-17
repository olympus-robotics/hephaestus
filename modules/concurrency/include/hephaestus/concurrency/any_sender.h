//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#include <exec/any_sender_of.hpp>
#include <stdexec/__detail/__execution_fwd.hpp>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

namespace heph::concurrency {
#ifdef DOXYGEN
/// Type-erased scheduler to be used as a stdexec::scheduler
struct AnyScheduler {};
#else
namespace internal {
using AnySchedulerCompletions =
    stdexec::completion_signatures<stdexec::set_error_t(std::exception_ptr), stdexec::set_stopped_t()>;
}
using AnyScheduler = exec::any_receiver_ref<internal::AnySchedulerCompletions>::any_sender<>::any_scheduler<>;
static_assert(stdexec::scheduler<AnyScheduler>);
#endif

/// Type erasing implementation for a stdexec environment forwarding a \ref AnyScheduler for
/// `stdexec::get_scheduler` and `stdexec::get_delegation_scheduler` as well as forwarding
/// `stdexec::get_stop_token`
class AnyEnv {
#ifndef DOXYGEN
  struct Base {
    virtual ~Base() = default;

    [[nodiscard]] virtual auto getScheduler() const -> AnyScheduler = 0;
    [[nodiscard]] virtual auto getStopToken() const -> stdexec::inplace_stop_token = 0;
  };

  template <typename Env>
  class Impl : public Base {
  public:
    template <typename EnvT>
      requires(std::is_same_v<Env, std::decay_t<EnvT>>)
    explicit Impl(EnvT&& env) : env_(std::forward<EnvT>(env)) {
    }
    [[nodiscard]] auto getScheduler() const -> AnyScheduler final {
      return stdexec::inline_scheduler{};  // stdexec::get_scheduler(env_);
    }

    /// Forwards the stop token of the passed environment. If `stdexec::get_stop_token`
    /// does not return `stdexec::inplace_stop_token`, returns a default constructed one which
    /// never stops.
    [[nodiscard]] auto getStopToken() const -> stdexec::inplace_stop_token final {
      auto stop_token = stdexec::get_stop_token(env_);
      using StopTokenT = decltype(stop_token);
      if constexpr (std::is_same_v<stdexec::inplace_stop_token, StopTokenT>) {
        return stop_token;
      } else {
        const static stdexec::inplace_stop_source stop_source;
        return stop_source.get_token();
      }
    }

  private:
    Env env_;
  };
#endif

public:
  template <typename Env, typename EnvT = std::decay_t<Env>>
    requires(!std::is_same_v<EnvT, AnyEnv>)
  explicit AnyEnv(Env&& env) : impl_(std::make_unique<Impl<EnvT>>(std::forward<Env>(env))) {
  }
  [[nodiscard]] static constexpr auto query(stdexec::__is_scheduler_affine_t /*ignore*/) noexcept {
    return true;
  }

  [[nodiscard]] auto query(stdexec::get_scheduler_t /*ignore*/) const noexcept -> AnyScheduler {
    return impl_->getScheduler();
  }

  [[nodiscard]] auto query(stdexec::get_delegation_scheduler_t /*ignore*/) const noexcept -> AnyScheduler {
    return impl_->getScheduler();
  }

  [[nodiscard]] auto query(stdexec::get_stop_token_t /*ignore*/) const noexcept
      -> stdexec::inplace_stop_token {
    return impl_->getStopToken();
  }

private:
  std::unique_ptr<Base> impl_;
};

#ifndef DOXYGEN
template <typename T>
class AnyReceiver {
  using TypeT = std::conditional_t<std::is_same_v<T, void>, decltype(std::ignore), T>;
  struct Base {
    virtual ~Base() = default;
    virtual void setValue(TypeT value) noexcept = 0;
    virtual void setStopped() noexcept = 0;
    virtual void setError(std::exception_ptr exception) noexcept = 0;
    [[nodiscard]] virtual auto getEnv() const noexcept -> AnyEnv = 0;
  };

  template <typename Receiver>
  class Impl : public Base {
  public:
    explicit Impl(Receiver&& receiver) : receiver_(std::move(receiver)) {
    }
    void setValue(TypeT value) noexcept final {
      if constexpr (std::is_same_v<T, void>) {
        stdexec::set_value(std::move(receiver_));
      } else {
        stdexec::set_value(std::move(receiver_), std::move(value));
      }
    }
    void setStopped() noexcept final {
      stdexec::set_stopped(std::move(receiver_));
    }
    void setError(std::exception_ptr exception) noexcept final {
      stdexec::set_error(std::move(receiver_), std::move(exception));
    }
    [[nodiscard]] auto getEnv() const noexcept -> AnyEnv final {
      return AnyEnv{ stdexec::get_env(receiver_) };
    }

  private:
    Receiver receiver_;
  };

public:
  using receiver_concept = stdexec::receiver_t;

  template <typename Receiver, typename ReceiverT = std::decay_t<Receiver>>
    requires(!std::is_same_v<ReceiverT, AnyReceiver>)
  explicit AnyReceiver(Receiver&& receiver)
    : receiver_(std::make_unique<Impl<ReceiverT>>(std::forward<Receiver>(receiver))) {
  }

  ~AnyReceiver() = default;
  AnyReceiver(AnyReceiver&& other) = default;
  auto operator=(AnyReceiver&& other) -> AnyReceiver& = default;
  AnyReceiver(const AnyReceiver& other) = delete;
  auto operator=(const AnyReceiver& other) -> AnyReceiver& = delete;

  //  NOLINTBEGIN (readability-identifier-naming) - wrapping stdexec interface
  template <typename... Ts>
  void set_value(Ts&&... ts) noexcept {
    if constexpr (std::is_same_v<T, void>) {
      static_assert(sizeof...(Ts) == 0);
      receiver_->setValue(std::ignore);
    } else {
      static_assert(sizeof...(Ts) == 1);
      receiver_->setValue(std::forward<Ts>(ts)...);
    }
  }
  void set_stopped() noexcept {
    receiver_->setStopped();
  }
  void set_error(std::exception_ptr ptr) noexcept {
    receiver_->setError(ptr);
  }

  [[nodiscard]] auto get_env() const noexcept -> AnyEnv {
    return { receiver_->getEnv() };
  }
  // NOLINTEND

private:
  std::unique_ptr<Base> receiver_;
};

class AnyOperation {
  struct Base {
    virtual ~Base() = default;
    virtual void start() noexcept = 0;
  };

  template <typename Sender, typename Receiver>
  class Impl : public Base {
    using OperationT = stdexec::connect_result_t<Sender&&, Receiver>;

  public:
    explicit Impl(Sender&& sender, Receiver receiver)
      : operation_(stdexec::connect(std::move(sender), std::move(receiver))) {
    }

    void start() noexcept final {
      stdexec::start(operation_);
    }

  private:
    OperationT operation_;
  };

public:
  template <typename Sender, typename Receiver>
  explicit AnyOperation(Sender&& sender, Receiver receiver)
    : impl_(std::make_unique<Impl<std::decay_t<Sender>, Receiver>>(std::forward<Sender>(sender),
                                                                   std::move(receiver))) {
  }
  ~AnyOperation() = default;
  AnyOperation(AnyOperation&& other) = delete;
  auto operator=(AnyOperation&& other) -> AnyOperation& = delete;
  AnyOperation(const AnyOperation& other) = delete;
  auto operator=(const AnyOperation& other) -> AnyOperation& = delete;

  void start() & noexcept {
    impl_->start();
  }

private:
  std::unique_ptr<Base> impl_;
};
#endif

template <typename T>
class AnySender;

template <typename T>
struct SelectValueCompletion {
  using TypeT = stdexec::set_value_t(T);
};
template <>
struct SelectValueCompletion<void> {
  using TypeT = stdexec::set_value_t();
};

template <typename T>
using SelectValueCompletionT = typename SelectValueCompletion<T>::TypeT;

template <typename Sender, typename T>
concept AnySenderRequirements =
    !std::is_same_v<std::decay_t<Sender>, AnySender<T>> &&
    (stdexec::sender_of<Sender, SelectValueCompletionT<T>, concurrency::AnyEnv> ||
     stdexec::sender_of<Sender, stdexec::set_stopped_t(), concurrency::AnyEnv> ||
     stdexec::sender_of<Sender, stdexec::set_error_t(std::exception_ptr), concurrency::AnyEnv>);

/// Implementation for a type erased sender.
///
/// Injects \ref AnyEnv to be able to schedule dependant tasks and react on cancellation requests
///
/// @tparam T The value this sender completes with
template <typename T>
class AnySender {
#ifndef DOXYGEN
  struct Base {
    virtual ~Base() = default;

    virtual auto doConnect(AnyReceiver<T> receiver) -> AnyOperation = 0;
  };
  template <typename Sender>
  struct Impl : Base {
    static_assert(!std::is_const_v<Sender>);
    static_assert(!std::is_reference_v<Sender>);
    Sender sender;
    template <typename SenderT>
      requires(std::is_same_v<std::decay_t<SenderT>, Sender>)
    explicit Impl(SenderT&& sender) : sender(std::forward<SenderT>(sender)) {
    }

    auto doConnect(AnyReceiver<T> receiver) -> AnyOperation final {
      return AnyOperation{ std::move(sender), std::move(receiver) };
    }
  };
#endif

public:
  using sender_concept = stdexec::sender_t;
  using completion_signatures =
      stdexec::completion_signatures<SelectValueCompletionT<T>, stdexec::set_error_t(std::exception_ptr),
                                     stdexec::set_stopped_t()>;

  template <AnySenderRequirements<T> Sender, typename SenderT = std::decay_t<Sender>>
  AnySender(Sender&& sender)  // NOLINT(google-explicit-constructor, hicpp-explicit-conversions)
    : impl_(std::make_unique<Impl<SenderT>>(std::forward<Sender>(sender))) {
  }

  ~AnySender() = default;
  AnySender(AnySender&& other) = default;
  auto operator=(AnySender&& other) -> AnySender& = default;
  AnySender(const AnySender& other) = delete;
  auto operator=(const AnySender& other) -> AnySender& = delete;

  template <stdexec::receiver_of<completion_signatures> Receiver>
  auto connect(Receiver&& receiver) {
    return impl_->doConnect(AnyReceiver<T>{ std::forward<Receiver>(receiver) });
  }

private:
  std::unique_ptr<Base> impl_;
};
}  // namespace heph::concurrency
