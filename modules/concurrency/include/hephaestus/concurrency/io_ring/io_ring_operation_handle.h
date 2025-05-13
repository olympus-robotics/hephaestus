//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdint>

#include "hephaestus/concurrency/io_ring/io_ring_operation_registration.h"

namespace heph::concurrency::io_ring {

template <typename IoRingOperationT>
struct IoRingOperationRegistrar {
  IoRingOperationRegistrar() noexcept = default;
  void instantiate() {
  }

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
    static auto index = IoRingOperationRegistry::instance().registerOperation<IoRingOperationT>();
    return index;
  }
};

}  // namespace heph::concurrency::io_ring
