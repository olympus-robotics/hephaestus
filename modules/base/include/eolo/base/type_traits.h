//=================================================================================================
// Copyright (C) 2018-2023 EOLO Contributors
//=================================================================================================

#pragma once

#include <type_traits>

namespace eolo {
/// \addtogroup base base module
/// @{

template <typename T>
concept ScalarType = requires(T a) { std::is_scalar_v<T>; };

///@}
}  // namespace eolo
