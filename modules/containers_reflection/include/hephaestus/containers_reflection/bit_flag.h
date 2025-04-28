//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <rfl.hpp>  // NOLINT(misc-include-cleaner)

#include "hephaestus/containers/bit_flag.h"

namespace rfl {
template <typename EnumT>
struct Reflector<heph::containers::BitFlag<EnumT>> {  // NOLINT(misc-include-cleaner)
  using ReflType = heph::containers::BitFlag<EnumT>::T;

  static auto from(const heph::containers::BitFlag<EnumT>& x) noexcept -> ReflType {
    return x.getUnderlyingValue();
  }

  static auto to(const ReflType& x) noexcept -> heph::containers::BitFlag<EnumT> {
    return heph::containers::BitFlag<EnumT>{ x };
  }
};
}  // namespace rfl
