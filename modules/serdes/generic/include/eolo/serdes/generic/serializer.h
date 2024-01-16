//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <memory>
#include <span>
#include <vector>

#include "eolo/base/exception.h"
#include "eolo/serdes/generic/concepts.h"
#include "eolo/utils/utils.h"

namespace eolo::serdes {

class Serializer {
public:
  /// Construct a generic serializer from a provided concrete serializer.
  /// \tparam A concrete serializer type.
  template <SerializerType S>
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  Serializer(S&& serializer)
    requires(!std::is_same_v<Serializer, std::decay_t<S>>)
    : pimpl_(std::make_unique<Model<S>>(std::forward<S>(serializer))) {
  }

  ~Serializer() noexcept = default;

  Serializer(const Serializer& other) : pimpl_(other.pimpl_->clone()) {
  }

  Serializer(Serializer&& other) noexcept = default;

  auto operator=(const Serializer& other) -> Serializer& {
    if (&other != this) {
      pimpl_ = other.pimpl_->clone();
    }
    return *this;
  }

  auto operator=(Serializer&& other) noexcept -> Serializer& = default;

  /// Serialize the data to a byte vector and returns it.
  /// \details This method allocates the memory for storing the serialized data.
  /// \return std::vector with the serialized data.
  [[nodiscard]] auto serialize() const -> std::vector<std::byte> {
    return pimpl_->serialize();
  }

  /// Get string representation of the underlying concrete serializer.
  [[nodiscard]] auto getSerializerType() const -> std::string {
    return pimpl_->getSerializerType();
  }

private:
  /// Type-erasure design pattern.
  struct Concept {
    virtual ~Concept() noexcept = default;

    /// Make a deep copy.
    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<Concept> = 0;

    /// Delegate serialize to a concrete serializer.
    [[nodiscard]] virtual auto serialize() const -> std::vector<std::byte> = 0;

    /// Return string representation of a concrete serializer type.
    [[nodiscard]] virtual auto getSerializerType() const -> std::string = 0;
  };

  template <typename S>
  struct Model final : public Concept {
    explicit Model(S&& serializer) : serializer_(std::move(serializer)) {
    }

    [[nodiscard]] auto clone() const -> std::unique_ptr<Concept> override {
      return std::make_unique<Model<S>>(*this);
    }

    [[nodiscard]] auto serialize() const -> std::vector<std::byte> override {
      return serializer_.serialize();
    }

    [[nodiscard]] auto getSerializerType() const -> std::string override {
      return utils::getTypeName<S>();
    }

  private:
    /// Decayed type of a concrete serializer.
    using DecayedS = std::decay_t<S>;

    /// Actual serializer which implements serialization logic.
    /// \note Always store a copy of a concrete serializer.
    DecayedS serializer_;
  };

  std::unique_ptr<Concept> pimpl_;
};

}  // namespace eolo::serdes
