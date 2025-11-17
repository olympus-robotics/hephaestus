//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/conduit/basic_input.h"
#include "hephaestus/conduit/scheduler.h"
#include "hephaestus/utils/unique_function.h"

namespace heph::conduit {

/// `Generator` represents an input which is producing values. A generator function (\ref setGenerator)
/// needs to be supplied before usage. Use this for ingesting data from source like network which can
/// make use of Sender/Receivers or coroutines.
///
/// @tparam T the type of the value to be generated
template <typename T>
struct Generator : public BasicInput {
public:
  explicit Generator(std::string_view name) : BasicInput(name) {
  }

  /// Provide a generator function which is called on `trigger`. Trigger will therefor complete once
  /// the returned sender of \ref func completes.
  ///
  /// Needs to be called before usage, otherwise `heph::conduit::BasicInput::trigger` will panic.
  ///
  /// @tparam Func generator function. Should either return T or a sender of T.
  template <typename Func>
  void setGenerator(Func func) {
    using ResultT = std::invoke_result_t<Func>;

    if constexpr (std::is_same_v<concurrency::AnySender<T>, ResultT>) {
      generator_ = std::move(func);
    } else if constexpr (concurrency::AnySenderRequirements<ResultT, T>) {
      generator_ = [func = std::move(func)]() -> concurrency::AnySender<T> { return func(); };
    } else {
      generator_ = [func = std::move(func)]() -> concurrency::AnySender<T> { return stdexec::just(func()); };
    }
  }

  auto hasValue() -> bool {
    return data_.has_value();
  }

  /// Retreives the generated value
  ///
  /// @throws heph::Panic if no value has been generated within this trigger round.
  [[nodiscard]] auto value() -> T {
    heph::panicIf(!data_.has_value(), "{}: No data available", name());
    std::optional<T> data;
    std::swap(data_, data);

    return *data;
  }

  template <typename... Ts>
  auto valueOr(Ts&&... ts) -> std::optional<T> {
    if (hasValue()) {
      return value();
    }
    return std::optional<T>{ std::forward<Ts>(ts)... };
  }

private:
  auto doTrigger(SchedulerT /*scheduler*/) -> SenderT final {
    heph::panicIf(!generator_, "{}: No generator function set", name());
    heph::panicIf(data_.has_value(), "{}: Data not consumed", name());
    data_.reset();
    return generator_() | stdexec::then([this](T value) {
             data_.emplace(std::move(value));
             return true;
           });
  }

  void handleCompleted() final {
  }

  void handleStopped() final {
    data_.reset();
  }

  void handleError() final {
    data_.reset();
  }

private:
  heph::UniqueFunction<concurrency::AnySender<T>()> generator_;
  std::optional<T> data_;
};
}  // namespace heph::conduit
