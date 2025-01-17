//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <array>
#include <chrono>
#include <concepts>
#include <ctime>
#include <future>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
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
concept StringLike =
    // Built-in string types
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view> ||
    std::same_as<std::remove_cvref_t<T>, const char*> ||
    // Array of chars (including string literals)
    (std::is_array_v<std::remove_reference_t<T>> &&
     std::same_as<std::remove_all_extents_t<std::remove_reference_t<T>>, char>);

template <typename T>
concept Formattable = requires(const T& t) {
  { t.format() } -> std::same_as<std::string>;
};

namespace internal {
// To allow ADL with custom begin/end
using std::begin;
using std::end;

template <typename T>
concept IsIterableImpl = requires(T& t) {
  begin(t) != end(t);                     // begin/end and operator !=
  ++std::declval<decltype(begin(t))&>();  // operator ++
  *begin(t);                              // operator*
};
}  // namespace internal

/// @brief Concept to check if a type is iterable.
template <typename T>
concept Iterable = internal::IsIterableImpl<T>;

// We want a SFINAE variant of this in order to be able to use it in the generic operator<< without falling
// into infinite recursion
template <typename T, typename = void>
struct has_stream_operator : std::false_type {};  // NOLINT(readability-identifier-naming)

template <typename T>
struct has_stream_operator<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
  : std::true_type {};

// Concept based on the SFINAE detection
template <typename T>
concept Streamable = has_stream_operator<T>::value;

template <typename T>
concept NonBooleanIntegralType = std::integral<T> && !BooleanType<T>;

template <typename T>
concept OptionalType = std::is_same_v<T, std::optional<typename T::value_type>>;

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
concept VectorOfArraysType = VectorType<T> && ArrayType<typename T::value_type>;

template <typename T>
concept VectorOfVectorsType = VectorType<T> && VectorType<typename T::value_type>;

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
concept CTimeType = std::is_same_v<std::tm, T>;

template <typename T>
concept TimeType = CTimeType<T> || ChronoTimestampType<T>;

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
