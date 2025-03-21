//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include "hephaestus/concurrency/io_ring_operation_registration.h"

namespace heph::concurrency {

template <typename IoRingOperationT>
struct IoRingOperationRegistrar {
  IoRingOperationRegistrar() noexcept
    : index(IoRingOperationRegistry::instance().registerOperation<IoRingOperationT>()) {
  }
  void instantiate() {
  }
  std::uint8_t index{ IoRingOperationRegistry::CAPACITY };

  static IoRingOperationRegistrar instance;
};

template <typename IoRingOperationT>
IoRingOperationRegistrar<IoRingOperationT> IoRingOperationRegistrar<IoRingOperationT>::instance{};

struct IoRingOperationBase {
  virtual ~IoRingOperationBase() = default;
  virtual void registerOperation() = 0;
};

template <typename IoRingOperationT>
struct IoRingOperationHandle : IoRingOperationBase {
  void registerOperation() final {
    IoRingOperationRegistrar<IoRingOperationT>::instance.instantiate();
  }

  [[nodiscard]] auto index() const -> std::uint8_t {
    return IoRingOperationRegistrar<IoRingOperationT>::instance.index;
  }
};

}  // namespace heph::concurrency
