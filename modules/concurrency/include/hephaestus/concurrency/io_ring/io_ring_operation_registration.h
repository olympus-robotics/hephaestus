//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <utility>

#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

namespace heph::concurrency::io_ring {
struct IoRingOperationIdentifierT {};

template <typename IoRingOperationT>
inline constexpr IoRingOperationIdentifierT IO_RING_OPERATION_IDENTIFIER{};

struct IoRingOperationRegistry {
  using identifier_function_t = bool (*)(void*);
  using prepare_function_t = void (*)(void*, ::io_uring_sqe*);
  using handle_completion_function_t = void (*)(void*, ::io_uring_cqe*);

  static constexpr std::uint8_t CAPACITY = 128;
  static_assert(CAPACITY <= std::numeric_limits<std::uint8_t>::max());

  std::uint8_t size{ 0 };
  std::array<void const*, CAPACITY> operation_identifier_table{ nullptr };
  std::array<prepare_function_t, CAPACITY> prepare_function_table{ nullptr };
  std::array<handle_completion_function_t, CAPACITY> handle_completion_function_table{ nullptr };

  template <typename IoRingOperationT>
  auto registerOperation() -> std::uint8_t {
    static constexpr void const* IDENTIFIER = &IO_RING_OPERATION_IDENTIFIER<IoRingOperationT>;
    // Check if already registered
    auto it = std::ranges::find_if(operation_identifier_table, [](void const* registered_identifier) {
      return registered_identifier == IDENTIFIER;
    });
    if (it != operation_identifier_table.end()) {
      return std::distance(operation_identifier_table.begin(), it);
    }

    prepare_function_t prepare{ nullptr };
    handle_completion_function_t handle_completion{ nullptr };
    if constexpr (requires() { std::declval<IoRingOperationT>().prepare(std::declval<::io_uring_sqe*>()); }) {
      prepare = [](void* operation, ::io_uring_sqe* sqe) {
        static_cast<IoRingOperationT*>(operation)->prepare(sqe);
      };
      handle_completion = [](void* operation, ::io_uring_cqe* cqe) {
        static_cast<IoRingOperationT*>(operation)->handleCompletion(cqe);
      };
    } else {
      handle_completion = [](void* operation, ::io_uring_cqe* /*cqe*/) {
        static_cast<IoRingOperationT*>(operation)->handleCompletion();
      };
    }
    const std::uint8_t idx{ size };
    registerOperation(idx, IDENTIFIER, prepare, handle_completion);

    ++size;

    return idx;
  }

  auto hasPrepare(std::uint8_t idx) -> bool;

  void prepare(std::uint8_t idx, void* operation, ::io_uring_sqe* sqe);

  void handleCompletion(std::uint8_t idx, void* operation, ::io_uring_cqe* cqe);

  static auto instance() -> IoRingOperationRegistry&;

private:
  void registerOperation(std::uint8_t idx, void const* identifier, prepare_function_t prepare,
                         handle_completion_function_t handle_completion);
};

}  // namespace heph::concurrency::io_ring
