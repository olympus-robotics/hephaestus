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

template <typename T>
concept EnumType = std::is_enum_v<T>;

template <typename T>
concept VectorType = requires {
  typename T::value_type;
  requires std::same_as<T, std::vector<typename T::value_type>>;
};

template <typename T>
concept UnorderedMapType = requires(T t) {
  typename T::key_type;
  typename T::mapped_type;
  typename T::hasher;
  typename T::key_equal;
  std::is_same_v<T, std::unordered_map<typename T::key_type, typename T::mapped_type, typename T::hasher,
                                       typename T::key_equal>>;
};

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
