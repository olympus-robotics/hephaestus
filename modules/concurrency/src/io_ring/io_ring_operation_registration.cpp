//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/concurrency/io_ring/io_ring_operation_registration.h"

#include <cstddef>
#include <cstdint>

#include <fmt/format.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/utils/exception.h"

namespace heph::concurrency::io_ring {
auto IoRingOperationRegistry::instance() -> IoRingOperationRegistry& {
  static IoRingOperationRegistry self;
  return self;
}
void IoRingOperationRegistry::handleCompletion(std::uint8_t idx, void* operation, ::io_uring_cqe* cqe) {
  heph::panicIf(idx >= size, fmt::format("Index out of range: {} >= {}", idx, size));
  handle_completion_function_table.at(idx)(operation, cqe);
}
void IoRingOperationRegistry::prepare(std::uint8_t idx, void* operation, ::io_uring_sqe* sqe) {
  heph::panicIf(idx >= size, fmt::format("Index out of range: {} >= {}", idx, size));
  prepare_function_table.at(idx)(operation, sqe);
}
auto IoRingOperationRegistry::hasPrepare(std::uint8_t idx) -> bool {
  heph::panicIf(idx >= size, fmt::format("Index out of range: {} >= {}", idx, size));
  return prepare_function_table.at(idx) != nullptr;
}
void IoRingOperationRegistry::registerOperation(std::uint8_t idx, void const* identifier,
                                                prepare_function_t prepare,
                                                handle_completion_function_t handle_completion) {
  heph::panicIf(idx >= CAPACITY, fmt::format("IoRingOperationRegistry exceeded capacity of {}", CAPACITY));
  operation_identifier_table.at(idx) = identifier;
  prepare_function_table.at(idx) = prepare;
  handle_completion_function_table.at(idx) = handle_completion;
}
}  // namespace heph::concurrency::io_ring
