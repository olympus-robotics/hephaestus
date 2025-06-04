//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>

namespace heph::containers {
struct IntrusiveFifoQueueAccess {
  template <typename T>
  static auto next(T* t) -> T*& {
    if constexpr (requires(T* node) { node->next; }) {
      return t->next;
    } else if constexpr (requires(T* node) { node->next_; }) {
      return t->next_;
    } else {
      static_assert(false, "Don't know how to access next pointer");
    }
  }
};

template <typename T>
class IntrusiveFifoQueue {
public:
  [[nodiscard]] auto empty() const -> bool {
    return head_ == nullptr;
  }

  [[nodiscard]] auto size() const -> std::size_t {
    return size_;
  }

  auto enqueue(T* t) {
    if (tail_ == nullptr) {
      head_ = t;
    } else {
      IntrusiveFifoQueueAccess::next(tail_) = t;
    }

    tail_ = t;
    ++size_;
  }

  auto dequeue() -> T* {
    if (head_ == nullptr) {
      return nullptr;
    }
    --size_;
    T* tmp = head_;
    head_ = IntrusiveFifoQueueAccess::next(head_);

    if (head_ == nullptr) {
      tail_ = nullptr;
    }

    IntrusiveFifoQueueAccess::next(tmp) = nullptr;

    return tmp;
  }

  auto erase(T* t) {
    if (empty() || t == nullptr) {
      return;
    }
    // Delete from front?
    if (t == head_) {
      dequeue();
      return;
    }
    // find the previous element
    T* prev = head_;
    while (prev != nullptr) {
      if (IntrusiveFifoQueueAccess::next(prev) == t) {
        break;
      }
      prev = IntrusiveFifoQueueAccess::next(prev);
    }
    if (prev == nullptr) {
      return;
    }

    IntrusiveFifoQueueAccess::next(prev) = IntrusiveFifoQueueAccess::next(t);
    IntrusiveFifoQueueAccess::next(t) = nullptr;
    --size_;
  }

private:
  T* head_{ nullptr };
  T* tail_{ nullptr };
  std::size_t size_{ 0 };
};
}  // namespace heph::containers
