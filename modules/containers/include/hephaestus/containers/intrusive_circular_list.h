//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>

#include "hephaestus/utils/exception.h"

namespace heph::containers {
class CircularListHook;
template <typename Node>
class IntrusiveCircularListIterator;

template <typename Node>
class IntrusiveCircularList {
public:
  IntrusiveCircularList() = default;
  IntrusiveCircularList(IntrusiveCircularList&) = delete;
  auto operator=(IntrusiveCircularList&) -> IntrusiveCircularList& = delete;
  IntrusiveCircularList(IntrusiveCircularList&&) = delete;
  auto operator=(IntrusiveCircularList&&) -> IntrusiveCircularList& = delete;
  ~IntrusiveCircularList() = default;
  static_assert(!std::is_copy_constructible_v<Node>);
  static_assert(!std::is_move_constructible_v<Node>);
  static_assert(!std::is_copy_assignable_v<Node>);
  static_assert(!std::is_move_assignable_v<Node>);

  using IteratorT = IntrusiveCircularListIterator<Node>;

  auto clear() noexcept -> void;
  auto empty() noexcept -> bool;
  auto size() noexcept -> std::size_t;
  auto insertAfter(IteratorT pos, Node& n) -> void;
  auto insertBefore(IteratorT pos, Node& n) -> void;
  auto pushBack(Node& n) -> void;
  auto pushFront(Node& n) -> void;
  auto erase(Node& n) -> void;
  auto begin() noexcept -> IteratorT;
  auto end() noexcept -> IteratorT;

  auto front() -> Node&;
  auto back() -> Node&;

private:
private:
  friend class CircularListHook;
  CircularListHook* head_{ nullptr };
  std::size_t size_{ 0 };
};

class CircularListHook {
public:
  template <typename Node>
  auto unlink() noexcept -> void {
    assert(charged());
    auto* self = this->self<Node>();
    if (self->size_ == 1) {
      self->clear();
      return;
    }
    unlinkImpl(&self->head_);
    --self->size_;
  }

  template <typename Node>
  auto linkBefore(IntrusiveCircularList<Node>& list, CircularListHook* next, Node& data) noexcept -> void {
    assert(!charged());
    self_ = &list;
    data_ = &data;

    linkBeforeImpl(&list.head_, next);
    ++list.size_;
  }

  template <typename Node>
  auto linkAfter(IntrusiveCircularList<Node>& list, CircularListHook* prev, Node& data) noexcept -> void {
    assert(!charged());
    self_ = &list;
    data_ = &data;

    linkAfterImpl(&list.head_, prev);
    ++list.size_;
  }

  auto next(CircularListHook* head) noexcept -> CircularListHook*;
  auto prev(CircularListHook* head) noexcept -> CircularListHook*;

private:
  template <typename Node>
  auto self() noexcept -> IntrusiveCircularList<Node>* {
    assert(charged());
    return static_cast<IntrusiveCircularList<Node>*>(self_);
  }
  template <typename Node>
  auto data() noexcept -> Node* {
    assert(charged());
    return static_cast<Node*>(data_);
  }

  void charge(void* self) noexcept;
  auto charged() noexcept -> bool;
  auto linkBeforeImpl(CircularListHook** head, CircularListHook* next) noexcept -> void;
  auto linkAfterImpl(CircularListHook** head, CircularListHook* prev) noexcept -> void;
  auto unlinkImpl(CircularListHook** head) noexcept -> void;

private:
  template <typename Node>
  friend class IntrusiveCircularList;
  template <typename Node>
  friend class IntrusiveCircularListIterator;

  void* self_{ nullptr };
  void* data_{ nullptr };
  CircularListHook* next_{ nullptr };
  CircularListHook* prev_{ nullptr };
};

template <typename Node>
auto IntrusiveCircularList<Node>::clear() noexcept -> void {
  for (Node& n : *this) {
    auto& hook = n.circularListHook();
    hook.self_ = nullptr;
    hook.data_ = nullptr;
    hook.next_ = nullptr;
    hook.prev_ = nullptr;
  }
  head_ = nullptr;
  size_ = 0;
}

template <typename Node>
auto IntrusiveCircularList<Node>::empty() noexcept -> bool {
  return head_ == nullptr;
}

template <typename Node>
auto IntrusiveCircularList<Node>::size() noexcept -> std::size_t {
  return size_;
}

template <typename Node>
auto IntrusiveCircularList<Node>::insertAfter(IteratorT pos, Node& n) -> void {
  auto& hook = n.circularListHook();
  heph::panicIf(hook.self_ != nullptr, "Node is already part of a list");
  heph::panicIf(pos.curr_ == nullptr || pos.curr_->self_ != this, "Got iterator into different list");
  hook.linkAfter(*this, pos.curr_, n);
}

template <typename Node>
auto IntrusiveCircularList<Node>::insertBefore(IteratorT pos, Node& n) -> void {
  auto& hook = n.circularListHook();
  heph::panicIf(hook.self_ != nullptr, "Node is already part of a list");
  heph::panicIf(pos.curr_ == nullptr || pos.curr_->self_ != this, "Got iterator into different list");
  hook.linkBefore(*this, pos.curr_, n);
}

template <typename Node>
auto IntrusiveCircularList<Node>::pushBack(Node& n) -> void {
  auto& hook = n.circularListHook();
  heph::panicIf(hook.self_ != nullptr, "Node is already part of a list");
  if (head_ == nullptr) {
    hook.linkAfter(*this, nullptr, n);
    return;
  }
  hook.linkAfter(*this, head_->prev_, n);
}

template <typename Node>
auto IntrusiveCircularList<Node>::pushFront(Node& n) -> void {
  auto& hook = n.circularListHook();
  heph::panicIf(hook.self_ != nullptr, "Node is already part of a list");
  hook.linkBefore(*this, head_, n);
}

template <typename Node>
auto IntrusiveCircularList<Node>::erase(Node& n) -> void {
  auto& hook = n.circularListHook();
  heph::panicIf(hook.self_ == nullptr || hook.self_ != this, "Node is not part of this list");
  hook.template unlink<Node>();
}

template <typename Node>
auto IntrusiveCircularList<Node>::begin() noexcept -> IteratorT {
  return IteratorT{ &head_, head_ };
}

template <typename Node>
auto IntrusiveCircularList<Node>::end() noexcept -> IteratorT {
  return IteratorT{ &head_, nullptr };
}

template <typename Node>
auto IntrusiveCircularList<Node>::front() -> Node& {
  heph::panicIf(empty(), "List is empty");
  return *head_->data<Node>();
}

template <typename Node>
auto IntrusiveCircularList<Node>::back() -> Node& {
  heph::panicIf(empty(), "List is empty");
  return *head_->prev_->data<Node>();
}
template <typename Node>
class IntrusiveCircularListIterator {
public:
  using difference_type = std::ptrdiff_t;
  using value_type = Node;
  IntrusiveCircularListIterator() noexcept = default;

  auto operator*() const -> Node& {
    heph::panicIf(curr_ == nullptr, "Attempt to dereference end iterator");
    return *curr_->template data<Node>();
  }

  auto operator++() noexcept -> IntrusiveCircularListIterator& {
    if (curr_ != nullptr) {
      curr_ = curr_->next(*head_);
    } else {
      curr_ = *head_;
    }
    return *this;
  }

  auto operator++(int) noexcept -> IntrusiveCircularListIterator {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  auto operator--() noexcept -> IntrusiveCircularListIterator& {
    if (curr_ != nullptr) {
      curr_ = curr_->prev((*head_)->prev_);
    } else {
      curr_ = (*head_)->prev_;
    }
    return *this;
  }

  auto operator--(int) noexcept -> IntrusiveCircularListIterator {
    auto tmp = *this;
    --(*this);
    return tmp;
  }

  auto operator==(const IntrusiveCircularListIterator& other) const -> bool {
    heph::panicIf(head_ != other.head_, "Attempt to compare iterators from different lists");
    return curr_ == other.curr_;
  }

  auto operator!=(const IntrusiveCircularListIterator& other) const -> bool {
    return !(*(this) == other);
  }

private:
  friend class IntrusiveCircularList<Node>;
  explicit IntrusiveCircularListIterator(CircularListHook** head, CircularListHook* curr) noexcept
    : head_{ head }, curr_{ curr } {
  }

private:
  CircularListHook** head_{ nullptr };
  CircularListHook* curr_{ nullptr };
};

}  // namespace heph::containers
