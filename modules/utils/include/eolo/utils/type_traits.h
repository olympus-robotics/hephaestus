//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <type_traits>

namespace eolo {

template <typename T>
concept ScalarType = requires(T a) { std::is_scalar_v<T>; };

}  // namespace eolo
