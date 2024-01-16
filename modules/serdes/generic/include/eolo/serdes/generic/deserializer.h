//=================================================================================================
// PIPPO © Copyright, 2015-2023. All Rights Reserved.
//
// This file is part of the RoboCool project
//=================================================================================================

#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "eolo/utils/type_traits.h"
#include "eolo/utils/utils.h"

namespace eolo::serdes {

template <typename DataType>
class Deserializer {
public:
  /// Construct a generic deserializer from a provided concrete deserializer.
  template <typename D>
  // NOLINTNEXTLINE(google-explicit-constructor,hicpp-explicit-conversions)
  Deserializer(D&& deserializer)
    requires(!std::is_same_v<Deserializer<DataType>, std::decay_t<D>>)
    : pimpl_(std::make_unique<Model<D>>(std::forward<D>(deserializer))) {
  }

  ~Deserializer() noexcept = default;

  Deserializer(const Deserializer& other) : pimpl_(other.pimpl_->clone()) {
  }

  Deserializer(Deserializer&& other) noexcept = default;

  auto operator=(const Deserializer& other) -> Deserializer& {
    if (&other != this) {
      pimpl_ = other.pimpl_->clone();
    }
    return *this;
  }

  auto operator=(Deserializer&& other) noexcept -> Deserializer& = default;

  /// Deserialize data from a given buffer.
  /// \param buffer Buffer containing serialized data.
  /// \note Buffer cannot be const, as one of the supported libraries, FastBuffer, requires
  /// non-const data.
  /// \return Newly constructed data object.
  [[nodiscard]] auto deserializeFromBuffer(std::span<std::byte> buffer) const -> std::optional<DataType> {
    return pimpl_->deserializeFromBuffer(buffer);
  }

  /// Get string representation of an underlying concrete serializer.
  [[nodiscard]] auto getDeserializerType() const -> std::string {
    return pimpl_->getDeserializerType();
  }

private:
  /// Type-erasure design pattern.
  struct Concept {
    virtual ~Concept() noexcept = default;

    /// Make a deep copy.
    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<Concept> = 0;

    /// Delegate deserializeFromBuffer to a concrete deserializer.
    [[nodiscard]] virtual auto deserializeFromBuffer(std::span<std::byte> buffer) const -> std::optional<DataType> = 0;

    /// Return string representation of a concrete deserializer type.
    [[nodiscard]] virtual auto getDeserializerType() const -> std::string = 0;
  };

  template <typename D>
  struct Model final : public Concept {
    explicit Model(D&& deserializer) : deserializer_(std::move(deserializer)) {
    }

    [[nodiscard]] auto clone() const -> std::unique_ptr<Concept> override {
      return std::make_unique<Model<D>>(*this);
    }

    [[nodiscard]] auto deserializeFromBuffer(std::span<std::byte> buffer) const -> std::optional<DataType> override {
      return deserializer_.deserializeFromBuffer(buffer);
    }

    [[nodiscard]] auto getDeserializerType() const -> std::string override {
      return utils::getTypeName<DecayedD>();
    }

  private:
    /// Decayed type of a concrete deserializer.
    using DecayedD = std::decay_t<D>;

    /// Actual deserializer which implements deserialization logic.
    /// \note Always store a copy of a concrete deserializer.
    DecayedD deserializer_;
  };

  std::unique_ptr<Concept> pimpl_;
};

}  // namespace eolo::serdes
