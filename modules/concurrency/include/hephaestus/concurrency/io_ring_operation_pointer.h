//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/concurrency/io_ring_operation_handle.h"
#include "hephaestus/concurrency/io_ring_operation_registration.h"

namespace heph::concurrency {

struct IoRingOperationPointer {
  static constexpr std::size_t IDX_MASK = 0x0000000000000007;
  static constexpr std::size_t IDX_SHIFT = 7;
  static constexpr std::size_t PTR_MASK = ~IDX_MASK;

  template <typename IoRingOperationT>
  explicit IoRingOperationPointer(IoRingOperationT* operation) {
    static_assert(alignof(IoRingOperationT) >= alignof(void*));

    static constexpr IoRingOperationHandle<IoRingOperationT> OPERATION{};
    auto idx = static_cast<std::size_t>(OPERATION.index());
    std::size_t ptr{ 0 };
    std::memcpy(&ptr, static_cast<const void*>(&operation), sizeof(void*));

    data = (idx | ptr);
    // ptr = static_cast<(index) | operation;
  }

  explicit IoRingOperationPointer(std::size_t data) : data(data) {
  }

  void prepare(::io_uring_sqe* sqe) const {
    IoRingOperationRegistry::instance().prepare(index(), pointer(), sqe);
  }

  void handleCompletion(::io_uring_cqe* cqe) const {
    IoRingOperationRegistry::instance().handleCompletion(index(), pointer(), cqe);
  }

  [[nodiscard]] auto index() const -> std::uint8_t {
    return data & IDX_MASK;
  }

  [[nodiscard]] auto pointer() const -> void* {
    auto ptr_int = data & PTR_MASK;
    void* res{ nullptr };
    std::memcpy(static_cast<void*>(&res), &ptr_int, sizeof(void*));
    return res;
  }

  std::size_t data{ 0 };
};
static_assert(sizeof(IoRingOperationPointer) == sizeof(void*));

}  // namespace heph::concurrency
