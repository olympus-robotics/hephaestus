//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <concepts>
#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>

namespace heph {
/// The `UniqueFunction` is very similar to `std::function` with the
/// only difference that it is move only (similar to `std::move_only_function`).
template <typename Sig>
class UniqueFunction;

namespace detail {
struct FunctorStorage {
  void* ptr{ nullptr };
  void* padding{ nullptr };
};

template <typename R, typename... Args>
auto emptyCall(FunctorStorage const& /**/, Args... /*unused*/) -> R {
  throw std::bad_function_call{};
}

inline void destroyEmpty(FunctorStorage const& /**/) noexcept {
}

inline void moveEmpty(FunctorStorage const& /**/, FunctorStorage& /**/) noexcept {
}

template <typename T>
static inline constexpr bool IS_INPLACE_ALLOCATED =
    // Check that it fits
    sizeof(T) <= sizeof(FunctorStorage)
    // and that it will be aligned
    && alignof(FunctorStorage) % alignof(T) == 0;

template <typename T>
auto isNull(T const& /**/) noexcept -> bool {
  return false;
}
template <typename R, typename... Args>
auto isNull(R (*func)(Args...)) noexcept -> bool {
  return func == nullptr;
}
template <typename R, typename Class, typename... Args>
auto isNull(R (Class::*func)(Args...)) noexcept -> bool {
  return func == nullptr;
}
template <typename R, typename Class, typename... Args>
auto isNull(R (Class::*func)(Args...) const) noexcept -> bool {
  return func == nullptr;
}

template <typename R, typename... Args>
struct Vtable {
  using invokeFunctionT = R (*)(FunctorStorage const&, Args...);
  using destroyFunctionT = void (*)(FunctorStorage const&) noexcept;
  using moveFunctionT = void (*)(FunctorStorage const&, FunctorStorage&) noexcept;

  invokeFunctionT invoke{ &emptyCall<R, Args...> };
  moveFunctionT move{ &moveEmpty };
  destroyFunctionT destroy{ &destroyEmpty };
};

template <typename F, typename R, typename... Args>
auto invokeFunction(FunctorStorage const& storage, Args... args) -> R {
  F* f{ nullptr };
  if constexpr (detail::IS_INPLACE_ALLOCATED<F>) {
    void const* storage_ptr = static_cast<void const*>(&storage);
    std::memcpy(&f, &storage_ptr, sizeof(void*));
  } else {
    f = static_cast<F*>(storage.ptr);
  }
  return (*f)(static_cast<Args&&>(args)...);
}

template <typename F>
void moveFunction(FunctorStorage const& source, FunctorStorage& destination) noexcept {
  if constexpr (detail::IS_INPLACE_ALLOCATED<F>) {
    if constexpr (std::is_trivially_move_constructible_v<F>) {
      destination = source;
    } else {
      F* f{ nullptr };
      void const* storage = static_cast<void const*>(&source);
      std::memcpy(&f, &storage, sizeof(void*));
      new (&destination) F{ std::move(*f) };
    }
  } else {
    // simply copy over the pointer, avoid a memory allocation
    destination.ptr = source.ptr;
  }
}

template <typename F>
void destroyFunction(FunctorStorage const& storage) noexcept {
  if constexpr (detail::IS_INPLACE_ALLOCATED<F>) {
    if constexpr (!std::is_trivially_destructible_v<F>) {
      F* f{ nullptr };
      void const* storage_ptr = static_cast<void const*>(&storage);
      std::memcpy(&f, &storage_ptr, sizeof(void*));
      f->~F();
    }
  } else {
    F* f = static_cast<F*>(storage.ptr);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete f;
  }
}
template <typename R, typename... Args>
inline constexpr Vtable<R, Args...> EMPTY_VTABLE{};

template <typename F, typename R, typename... Args>
inline constexpr Vtable<R, Args...> VTABLE{ .invoke = &invokeFunction<F, R, Args...>,
                                            .move = &moveFunction<F>,
                                            .destroy = &destroyFunction<F> };

template <typename F, typename R, typename... Args>
concept invocable = !std::is_same_v<F, UniqueFunction<R(Args...)>> && std::is_move_constructible_v<F> &&
                    std::invocable<F, Args...> && std::is_same_v<R, std::invoke_result_t<F, Args...>>;

}  // namespace detail

template <typename R, typename... Args>
class UniqueFunction<R(Args...)> {
public:
  UniqueFunction() noexcept : vtable_{ &detail::EMPTY_VTABLE<R, Args...> } {
  }

  ~UniqueFunction() noexcept {
    // Delete contained object. This is safe since the empty vtable
    // is a noop.
    vtable_->destroy(storage_);
  }

  // NOLINTBEGIN(google-explicit-constructor)
  // NOLINTBEGIN(hicpp-explicit-conversions)
  // We use non-explicit construction here to allow for example automatic conversion of lambdas.
  UniqueFunction(std::nullptr_t /**/) noexcept : vtable_{ &detail::EMPTY_VTABLE<R, Args...> } {
  }

  template <detail::invocable<R, Args...> F>
  UniqueFunction(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>)
    : vtable_{ &detail::VTABLE<std::decay_t<F>, R, Args...> } {
    initializeStorage(std::forward<F>(f));
  }
  // NOLINTEND(hicpp-explicit-conversions)
  // NOLINTEND(google-explicit-constructor)

  UniqueFunction(UniqueFunction const& other) noexcept = delete;
  UniqueFunction(UniqueFunction&& other) noexcept : vtable_{ other.vtable_ } {
    other.vtable_->move(other.storage_, storage_);
    other.vtable_ = &detail::EMPTY_VTABLE<R, Args...>;
  }
  auto operator=(UniqueFunction const& other) noexcept -> UniqueFunction& = delete;
  auto operator=(UniqueFunction&& other) noexcept -> UniqueFunction& {
    if (this == &other) {
      return *this;
    }

    // Delete contained object. This is save since the empty vtable
    // is a noop.
    vtable_->destroy(storage_);

    vtable_ = other.vtable_;
    other.vtable_->move(other.storage_, storage_);
    other.vtable_ = &detail::EMPTY_VTABLE<R, Args...>;
  }

  template <detail::invocable<R, Args...> F>
  auto operator=(F&& f) noexcept -> UniqueFunction& {
    vtable_->destroy(storage_);

    vtable_ = &detail::VTABLE<std::decay_t<F>, R, Args...>;
    initializeStorage(std::forward<F>(f));

    return *this;
  }

  auto operator=(std::nullptr_t /**/) noexcept -> UniqueFunction& {
    vtable_->destroy(storage_);
    vtable_ = &detail::EMPTY_VTABLE<R, Args...>;
    return *this;
  }

  auto operator()(Args... args) -> R {
    return vtable_->invoke(storage_, static_cast<Args&&>(args)...);
  }

  auto operator()(Args... args) const -> R {
    return vtable_->invoke(storage_, static_cast<Args&&>(args)...);
  }

  explicit operator bool() const noexcept {
    return vtable_ != &detail::EMPTY_VTABLE<R, Args...>;
  }

private:
  template <typename F>
  void initializeStorage(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) {
    if (detail::isNull(f)) {
      vtable_ = &detail::EMPTY_VTABLE<R, Args...>;
      return;
    }
    if constexpr (detail::IS_INPLACE_ALLOCATED<F>) {
      new (&storage_) std::decay_t<F>{ std::forward<F>(f) };
    } else {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      storage_.ptr = new std::decay_t<F>{ std::forward<F>(f) };
    }
  }

private:
  detail::Vtable<R, Args...> const* vtable_;
  detail::FunctorStorage storage_;
};

template <typename Sig>
auto operator==(UniqueFunction<Sig> const& func, std::nullptr_t /**/) -> bool {
  return !func;
}
template <typename Sig>
auto operator==(std::nullptr_t /**/, UniqueFunction<Sig> const& func) -> bool {
  return !func;
}

template <typename Sig>
auto operator!=(UniqueFunction<Sig> const& func, std::nullptr_t /**/) -> bool {
  return func;
}
template <typename Sig>
auto operator!=(std::nullptr_t /**/, UniqueFunction<Sig> const& func) -> bool {
  return func;
}
}  // namespace heph
