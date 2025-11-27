//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace heph::conduit {

/// ValueStorage is instantiated from a particular Value Storage Policy.
///
/// @tparam T type of stored value
template <typename T>
class ValueStorage {
#ifndef DOXYGEN
  struct ValueStorageBase {
    virtual ~ValueStorageBase() = default;
    [[nodiscard]] virtual auto hasValue() const -> bool = 0;
    [[nodiscard]] virtual auto value() -> T = 0;
    virtual void setValue(T&& t) = 0;
  };
  template <typename ValueStorageImplT>
  struct ValueStorageImpl : ValueStorageBase {
    explicit ValueStorageImpl(ValueStorageImplT outer_impl) : impl(std::move(outer_impl)) {
    }
    [[nodiscard]] auto hasValue() const -> bool final {
      return impl.hasValue();
    }

    [[nodiscard]] auto value() -> T final {
      return impl.value();
    }
    void setValue(T&& t) final {
      return impl.setValue(std::move(t));
    }
    ValueStorageImplT impl;
  };
#endif

public:
  template <typename ValueStoragePolicy>
    requires(!std::is_same_v<ValueStorage, std::remove_const_t<ValueStoragePolicy>>)
  explicit ValueStorage(ValueStoragePolicy impl)
    : storage_(std::make_unique<ValueStorageImpl<ValueStoragePolicy>>(std::move(impl))) {
  }

  /// @return true if a value is stored, false otherwise
  [[nodiscard]] auto hasValue() const -> bool {
    return storage_->hasValue();
  }

  /// @return the value stored
  [[nodiscard]] auto value() -> T {
    return storage_->value();
  }

  /// sets the value according to the policy
  void setValue(T&& t) {
    storage_->setValue(std::move(t));
  }

private:
  std::unique_ptr<ValueStorageBase> storage_;
};
}  // namespace heph::conduit
