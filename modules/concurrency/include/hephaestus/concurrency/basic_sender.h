//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <utility>

#include <stdexec/__detail/__sender_introspection.hpp>
#include <stdexec/execution.hpp>

namespace heph::concurrency {

struct Ignore {
  Ignore() noexcept = default;
  template <typename... T>
  Ignore(T&&... /*ignore*/) noexcept {  // NOLINT(google-explicit-constructor,hicpp-explicit-conversions)
  }
};

template <typename Function, typename... Ts>
using CallResultT = stdexec::__call_result_t<Function, Ts...>;

template <typename SenderExpressionT>
using DataOfT = stdexec::__data_of<SenderExpressionT>;

template <typename T>
struct Tag {};

struct DefaultSenderExpressionImpl {
  static constexpr auto GET_ATTRS = stdexec::__sexpr_defaults::get_attrs;
  static constexpr auto GET_ENV = stdexec::__sexpr_defaults::get_env;
  static constexpr auto GET_STATE = stdexec::__sexpr_defaults::get_state;
  static constexpr auto CONNECT = stdexec::__sexpr_defaults::connect;
  static constexpr auto START = stdexec::__sexpr_defaults::start;
  static constexpr auto COMPLETE = stdexec::__sexpr_defaults::complete;
};

template <typename TagT>
struct SenderExpressionImpl;

template <typename TagT, typename Data = stdexec::__, typename... Children>
constexpr auto makeSenderExpression(Data&& data = {}, Children&&... children) {
  return stdexec::__make_sexpr<Tag<TagT>>(std::forward<Data>(data), std::forward<Children>(children)...);
}

}  // namespace heph::concurrency

namespace stdexec {
template <typename TagT>
struct __sexpr_impl<::heph::concurrency::Tag<TagT>> {
  using Impl = ::heph::concurrency::SenderExpressionImpl<TagT>;
  static_assert(requires() { Impl::GET_COMPLETION_SIGNATURES; });
  //  NOLINTBEGIN(readability-identifier-naming) - wrapping stdexec interface
  static constexpr auto get_completion_signatures = Impl::GET_COMPLETION_SIGNATURES;
  static constexpr auto get_attrs = Impl::GET_ATTRS;
  static constexpr auto get_env = Impl::GET_ENV;
  static constexpr auto get_state = Impl::GET_STATE;
  static constexpr auto connect = Impl::CONNECT;
  static constexpr auto start = Impl::START;
  static constexpr auto complete = Impl::COMPLETE;
  // NOLINTEND(readability-identifier-naming)
};

}  // namespace stdexec
