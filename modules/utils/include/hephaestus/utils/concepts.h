//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <future>
#include <sstream>
#include <type_traits>

namespace heph {

template <typename T>
concept ScalarType = requires(T a) { std::is_scalar_v<T>; };

/// Types that are convertable to and from a string
template <typename T>
concept StringStreamable = requires(std::string str, T value) {
  std::istringstream{ str } >> value;
  std::ostringstream{ str } << value;
};

template <typename T>
concept Stoppable = requires(T value) {
  { value.stop() } -> std::same_as<std::future<void>>;
};

template <typename T>
concept Waitable = requires(T value) {
  { value.wait() };
};

}  // namespace heph
