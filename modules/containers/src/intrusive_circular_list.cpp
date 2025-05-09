//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/containers/intrusive_circular_list.h"

namespace heph::containers {
auto CircularListHook::charged() noexcept -> bool {
  return self_ != nullptr && data_ != nullptr;
}

auto CircularListHook::next(CircularListHook* head) noexcept -> CircularListHook* {
  if (next_ == head) {
    return nullptr;
  }
  return next_;
}
auto CircularListHook::prev(CircularListHook* head) noexcept -> CircularListHook* {
  if (prev_ == head) {
    return nullptr;
  }
  return prev_;
}

auto CircularListHook::linkBeforeImpl(CircularListHook** head, CircularListHook* next) noexcept -> void {
  // empty list
  if (*head == nullptr) {
    next_ = this;
    prev_ = this;
    *head = this;
    return;
  }
  // Before:
  //  next:  +---------v  +---------v
  //       | 0 |      |next|      | 1 |
  //  prev:  ^---------+  ^---------+
  //
  // After:
  //  next:  +---------v  +---------v  +---------v
  //       | 0 |      |this|       |next|      | 1 |
  //  prev:  ^---------+  ^---------+  ^---------+
  next_ = next;
  prev_ = next->prev_;
  prev_->next_ = this;
  next->prev_ = this;

  // Update head if needed...
  if (next == *head) {
    *head = this;
  }
}

auto CircularListHook::linkAfterImpl(CircularListHook** head, CircularListHook* prev) noexcept -> void {
  // empty list
  if (*head == nullptr) {
    next_ = this;
    prev_ = this;
    *head = this;
    return;
  }
  // Before:
  //  next:  +---------v  +---------v
  //       | 0 |      |prev|      | 1 |
  //  prev:  ^---------+  ^---------+
  //
  // After:
  //  next:  +---------v  +---------v  +---------v
  //       | 0 |      |prev|       |this|      | 1 |
  //  prev:  ^---------+  ^---------+  ^---------+
  prev_ = prev;
  next_ = prev->next_;
  next_->prev_ = this;
  prev->next_ = this;
}

auto CircularListHook::unlinkImpl(CircularListHook** head) noexcept -> void {
  self_ = nullptr;
  data_ = nullptr;
  auto* prev = prev_;
  auto* next = next_;
  prev_ = nullptr;
  next_ = nullptr;

  // Before:
  //  next:  +---------v  +---------v
  //       | 0 |      |this|      | 1 |
  //  prev:  ^---------+  ^---------+
  // After:
  //  next:  +---------v
  //       | 0 |     | 1 |
  //  prev:  ^---------+

  prev->next_ = next;
  next->prev_ = prev;

  // Update head if needed...
  if (this == *head) {
    *head = next;
    return;
  }
}
}  // namespace heph::containers
