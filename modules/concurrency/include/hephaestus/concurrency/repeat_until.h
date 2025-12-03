//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <utility>

#include <exec/repeat_effect_until.hpp>
#include <stdexec/__detail/__senders_core.hpp>
#include <stdexec/execution.hpp>

namespace heph::concurrency {
namespace internal {

template <typename SenderFactoryT>
concept SenderFactory = requires(SenderFactoryT sender_factory) {
  { sender_factory() } -> stdexec::sender;
};
}  // namespace internal

/// repeatedly starts \param sender_factory until the completion of the sender returns `true`.
///
/// \param sender_factory nullary function object returning a sender with a bool completion set_value
/// completion.
///
/// This can be used to implement loops using sender receivers. By passing a factory instead of the sender
/// directly, we are able to use movable only senders.
template <internal::SenderFactory SenderFactoryT>
auto repeatUntil(SenderFactoryT&& factory) {
  // 1. Create a lazy generator that calls the factory
  return stdexec::just() |
         stdexec::let_value([factory = std::forward<SenderFactoryT>(factory)]() mutable {
           // 2. Call the factory and normalize errors *inside* the loop
           return factory();  // | stdexec::let_error(internal::ErrorNormalizer{});
         })
         // 3. Repeat until the sender returns 'true'
         | exec::repeat_effect_until();
}

/// Convenience wrapper for \ref repeatUntil for cases where \param sender is copyable
template <stdexec::sender SenderT>
auto repeatUntil(SenderT&& sender) {
  return std::forward<SenderT>(sender) | exec::repeat_effect_until();
}
}  // namespace heph::concurrency
