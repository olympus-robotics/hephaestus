//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <concepts>
#include <cstddef>

namespace heph::containers {
struct IntrusiveFifoQueueAccess {
  template <typename T>
    requires requires(T) { T::next; }
  static auto next(T* t) -> T*& {
    return t->next;
  }
  template <typename T>
    requires requires(T) { T::next_; }
  static auto next(T* t) -> T*& {
    return t->next_;
  }
  template <typename T>
    requires requires(T) { T::prev; }
  static auto prev(T* t) -> T*& {
    return t->prev;
  }
  template <typename T>
    requires requires(T) { T::prev_; }
  static auto prev(T* t) -> T*& {
    return t->prev_;
  }
};

template <typename T>
concept IntrusiveFifoQueueElement = requires(T t) {
  { IntrusiveFifoQueueAccess::next(&t) } -> std::same_as<T*&>;
  { IntrusiveFifoQueueAccess::prev(&t) } -> std::same_as<T*&>;
};

template <IntrusiveFifoQueueElement T>
class IntrusiveFifoQueue {
public:
  [[nodiscard]] auto empty() const -> bool {
    return head_ == nullptr;
  }

  [[nodiscard]] auto size() const -> std::size_t {
    return size_;
  }

  auto enqueue(T* t) {
    ++size_;

    // Linked list is empty
    if (head_ == nullptr) {
      IntrusiveFifoQueueAccess::prev(t) = t;
      IntrusiveFifoQueueAccess::next(t) = t;
      head_ = t;
      return;
    }
    auto* tail = IntrusiveFifoQueueAccess::prev(head_);

    IntrusiveFifoQueueAccess::next(t) = head_;
    IntrusiveFifoQueueAccess::prev(t) = tail;

    IntrusiveFifoQueueAccess::next(tail) = t;
    IntrusiveFifoQueueAccess::prev(head_) = t;
  }

  auto dequeue() -> T* {
    if (head_ == nullptr) {
      return nullptr;
    }
    --size_;

    auto* tmp = head_;

    // Last element...
    if (size_ == 0) {
      head_ = nullptr;
      IntrusiveFifoQueueAccess::next(tmp) = nullptr;
      IntrusiveFifoQueueAccess::prev(tmp) = nullptr;
      return tmp;
    }

    auto* tail = IntrusiveFifoQueueAccess::prev(head_);

    head_ = IntrusiveFifoQueueAccess::next(head_);

    IntrusiveFifoQueueAccess::next(tail) = head_;
    IntrusiveFifoQueueAccess::prev(head_) = tail;

    IntrusiveFifoQueueAccess::next(tmp) = nullptr;
    IntrusiveFifoQueueAccess::prev(tmp) = nullptr;
    return tmp;
  }

  auto erase(T* t) -> bool {
    if (empty() || t == nullptr) {
      return false;
    }
    // Delete from front?
    if (t == head_) {
      dequeue();
      return true;
    }

    auto* prev = IntrusiveFifoQueueAccess::prev(t);
    auto* next = IntrusiveFifoQueueAccess::next(t);

    if (prev == nullptr || next == nullptr) {
      return false;
    }

    if (IntrusiveFifoQueueAccess::next(prev) != t) {
      return false;
    }
    if (IntrusiveFifoQueueAccess::prev(next) != t) {
      return false;
    }

    // Check if last node...
    if (IntrusiveFifoQueueAccess::next(t) == head_) {
      IntrusiveFifoQueueAccess::next(prev) = head_;
      IntrusiveFifoQueueAccess::prev(head_) = prev;
    } else {
      auto* tmp = IntrusiveFifoQueueAccess::next(t);

      IntrusiveFifoQueueAccess::next(prev) = tmp;
      IntrusiveFifoQueueAccess::prev(tmp) = prev;
    }

    --size_;
    IntrusiveFifoQueueAccess::next(t) = nullptr;
    IntrusiveFifoQueueAccess::prev(t) = nullptr;
    return true;
  }

private:
  T* head_{ nullptr };
  std::size_t size_{ 0 };
};
}  // namespace heph::containers
