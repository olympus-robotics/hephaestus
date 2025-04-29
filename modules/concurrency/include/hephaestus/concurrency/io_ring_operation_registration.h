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

#include <fmt/format.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/utils/exception.h"

namespace heph::concurrency {
struct IoRingOperationIdentifierT {};

template <typename IoRingOperationT>
static inline constexpr IoRingOperationIdentifierT IO_RING_OPERATION_IDENTIFIER{};

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
    std::uint8_t idx{ size };

    heph::panicIf(size == CAPACITY, fmt::format("IoRingOperationRegistry exceeded capacity of {}", CAPACITY));
    ++size;

    operation_identifier_table.at(idx) = IDENTIFIER;
    if constexpr (requires() { std::declval<IoRingOperationT>().prepare(std::declval<::io_uring_sqe*>()); }) {
      prepare_function_table.at(idx) = [](void* operation, ::io_uring_sqe* sqe) {
        static_cast<IoRingOperationT*>(operation)->prepare(sqe);
      };
      handle_completion_function_table.at(idx) = [](void* operation, ::io_uring_cqe* cqe) {
        static_cast<IoRingOperationT*>(operation)->handleCompletion(cqe);
      };
    } else {
      prepare_function_table.at(idx) = nullptr;
      handle_completion_function_table.at(idx) = [](void* operation, ::io_uring_cqe* /*cqe*/) {
        static_cast<IoRingOperationT*>(operation)->handleCompletion();
      };
    }

    return idx;
  }

  auto hasPrepare(std::uint8_t idx) {
    heph::panicIf(idx >= size, fmt::format("Index out of range: {} >= {}", idx, size));
    return prepare_function_table.at(idx) != nullptr;
  }

  void prepare(std::uint8_t idx, void* operation, ::io_uring_sqe* sqe) {
    heph::panicIf(idx >= size, fmt::format("Index out of range: {} >= {}", idx, size));
    prepare_function_table.at(idx)(operation, sqe);
  }

  void handleCompletion(std::uint8_t idx, void* operation, ::io_uring_cqe* cqe) {
    heph::panicIf(idx >= size, fmt::format("Index out of range: {} >= {}", idx, size));
    handle_completion_function_table.at(idx)(operation, cqe);
  }

  static auto instance() -> IoRingOperationRegistry& {
    static IoRingOperationRegistry self;
    return self;
  }
};

}  // namespace heph::concurrency
