//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <future>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace heph {

template <typename T>
concept ScalarType = std::is_scalar_v<T>;

template <typename T>
concept EnumType = std::is_enum_v<T>;

template <typename T>
concept BooleanType = std::is_same_v<T, bool>;

template <typename T>
concept StringType = std::is_same_v<T, std::string>;

template <typename T>
concept ArrayType = requires {
  typename T::value_type;
  requires std::same_as<T, std::array<typename T::value_type, T().size()>>;
};

template <typename T>
concept VectorType = requires {
  typename T::value_type;
  typename T::allocator_type;
  requires std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;
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

template <typename T>
concept ChronoSystemClockType = std::is_same_v<T, std::chrono::system_clock>;

template <typename T>
concept ChronoSteadyClockType = std::is_same_v<T, std::chrono::steady_clock>;

template <typename T>
concept ChronoClock = ChronoSystemClockType<T> || ChronoSteadyClockType<T>;

template <typename T>
concept ChronoSystemClockTimestampType = requires {
  typename T::clock;
  typename T::duration;
  requires std::is_same_v<typename T::clock, std::chrono::system_clock>;
  requires std::is_same_v<T, typename std::chrono::time_point<typename T::clock, typename T::duration>>;
};

template <typename T>
concept ChronoSteadyClockTimestampType = requires {
  typename T::clock;
  typename T::duration;
  requires std::is_same_v<typename T::clock, std::chrono::steady_clock>;
  requires std::is_same_v<T, typename std::chrono::time_point<typename T::clock, typename T::duration>>;
};

template <typename T>
concept ChronoTimestampType = ChronoSystemClockTimestampType<T> || ChronoSteadyClockTimestampType<T>;

template <typename T>
concept NumericType = (std::integral<T> || std::floating_point<T>) && !std::same_as<T, bool>;

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

template <typename T>
concept StoppableAndWaitable = requires { Stoppable<T>&& Waitable<T>; };

}  // namespace heph
