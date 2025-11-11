//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <random>
#include <string>

#include <magic_enum.hpp>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/utils/concepts.h"

namespace heph::random {

template <NumericType T>
struct Limits {
  T min;
  T max;
};

template <NumericType T>
constexpr Limits<T> NO_LIMITS{ .min = std::numeric_limits<T>::min(), .max = std::numeric_limits<T>::max() };

//=================================================================================================
// Random boolean creation
//=================================================================================================
template <BooleanType T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::bernoulli_distribution dist;
  return dist(mt);
}

//=================================================================================================
// Random integer value creation
//=================================================================================================
template <NonBooleanIntegralType T>
[[nodiscard]] auto random(std::mt19937_64& mt, Limits<T> limits = NO_LIMITS<T>) -> T {
  std::uniform_int_distribution<T> dist(limits.min, limits.max);
  return dist(mt);
}

//=================================================================================================
// Random floating point value creation
//=================================================================================================
template <std::floating_point T>
[[nodiscard]] auto random(std::mt19937_64& mt, Limits<T> limits = NO_LIMITS<T>) -> T {
  std::uniform_real_distribution<T> dist(limits.min, limits.max);
  return dist(mt);
}

//=================================================================================================
// Random floating point value creation
//=================================================================================================
template <typename T>
  requires(std::is_same_v<T, std::byte>)
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::uniform_int_distribution<std::uint8_t> dist;
  return static_cast<std::byte>(dist(mt));
}

//=================================================================================================
// Random enum creation
//=================================================================================================
template <EnumType T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  // We deduce the actual enum_values, which might not be monotonic, and then randomly select one.
  static const auto enum_values = magic_enum::enum_values<T>();
  std::uniform_int_distribution<size_t> dist(0, enum_values.size() - 1);
  return enum_values[dist(mt)];  // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

//=================================================================================================
// Random timestamp creation
//=================================================================================================
namespace internal {
template <ChronoTimestampType T, size_t Year>
[[nodiscard]] constexpr auto createFinalTimestampOfTheYear() -> T {
  // The final date of the year is YYYY-12-31.
  constexpr auto YEAR = std::chrono::year{ Year };
  constexpr auto FINAL_MONTH = std::chrono::December;
  constexpr auto FINAL_DAY = std::chrono::year_month_day_last{ YEAR / FINAL_MONTH / std::chrono::last };
  constexpr auto FINAL_DATE_SYS_DAYS = std::chrono::sys_days{ FINAL_DAY };

  // The final time of the day is 23:59:59.999...
  constexpr auto FINAL_TIME = std::chrono::hours{ 24 } - typename T::duration{ 1 };
  // The final timestamp of the year is YYYY-12-31 23:59:59.999...
  constexpr auto TOTAL_TIME = FINAL_DATE_SYS_DAYS + FINAL_TIME;

  return T{ TOTAL_TIME.time_since_epoch() };
}
}  // namespace internal

/// Create a random timestamp between year 1970 and the year 2100.
template <ChronoTimestampType T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  static constexpr auto MIN_DURATION = 0;  // Start of UNIX epoch time == year 1970.
  static constexpr auto MAX_YEAR = 2100;
  static constexpr auto FINAL_TIMESTAMP = internal::createFinalTimestampOfTheYear<T, MAX_YEAR>();
  static constexpr auto MAX_DURATION = FINAL_TIMESTAMP.time_since_epoch().count();  // End of year 2100.

  std::uniform_int_distribution<int64_t> duration_dist(MIN_DURATION, MAX_DURATION);
  const auto duration = typename T::duration{ duration_dist(mt) };
  const auto timestamp = T{ duration };

  return timestamp;
}

//=================================================================================================
// Random struct/class creation
//=================================================================================================
template <class T>
concept HasRandomMethod = requires(std::mt19937_64& mt) {
  { T::random(mt) } -> std::convertible_to<T>;
};

template <HasRandomMethod T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  return T::random(mt);
}

//=================================================================================================
// Concept for random creatable types
//=================================================================================================
template <class T>
concept RandomCreatable = requires(std::mt19937_64& mt) {
  { random<T>(mt) } -> std::convertible_to<T>;
};

//=================================================================================================
// Random optional creation for types with random free function
//=================================================================================================
template <typename T>
  requires(OptionalType<T> && RandomCreatable<typename T::value_type> &&
           !HasRandomMethod<typename T::value_type>)
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::bernoulli_distribution dist;
  const auto has_value = dist(mt);
  if (has_value) {
    return std::make_optional(random<typename T::value_type>(mt));
  }

  return std::nullopt;
}

//=================================================================================================
// Random optional creation for types with random method
//=================================================================================================
template <typename T>
  requires(OptionalType<T> && HasRandomMethod<typename T::value_type> &&
           !RandomCreatable<typename T::value_type>)
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::bernoulli_distribution dist;
  const auto has_value = dist(mt);
  if (has_value) {
    return std::make_optional(T::value_type::random(mt));
  }

  return std::nullopt;
}

//=================================================================================================
// Random optional creation for types that have both
//=================================================================================================
/// TODO(@graeter) This is a workaround until we have cpp23 and can introduce the
/// generic_random_object_creator
template <typename T>
  requires(OptionalType<T> && RandomCreatable<typename T::value_type> &&
           HasRandomMethod<typename T::value_type>)
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  std::bernoulli_distribution dist;
  const auto has_value = dist(mt);
  if (has_value) {
    return std::make_optional(random<typename T::value_type>(mt));
  }

  return std::nullopt;
}

//=================================================================================================
// Internal helper functions for container types
//=================================================================================================
namespace internal {
[[nodiscard]] inline auto getSize(std::mt19937_64& mt, std::optional<size_t> fixed_size, bool allow_empty)
    -> size_t {
  if (fixed_size.has_value()) {
    panicIf(allow_empty == false && fixed_size.value() == 0,
            "fixed_size must be non-zero if allow_empty == true");
    return fixed_size.value();
  }

  static constexpr auto MAX_SIZE = 42ul;
  const auto min_size = allow_empty ? 0ul : 1ul;
  std::uniform_int_distribution<size_t> dist(min_size, MAX_SIZE);

  return dist(mt);
}
}  // namespace internal

//=================================================================================================
// Random string creation
//=================================================================================================
/// Generate a random string of characters, including special case characters and numbers.
template <StringType T>
[[nodiscard]] auto random(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                          bool allow_empty = true, bool lower_characters_only = false) -> T {
  static constexpr auto PRINTABLE_ASCII_START = 32;         // Space
  static constexpr auto PRINTABLE_ASCII_END = 126;          // Equivalency sign - tilde
  static constexpr auto LOWER_CHARACTERS_ASCII_START = 97;  // a
  static constexpr auto LOWER_CHARACTERS_ASCII_END = 122;   // z

  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  auto char_dist =
      lower_characters_only ?
          std::uniform_int_distribution<unsigned char>(LOWER_CHARACTERS_ASCII_START,
                                                       LOWER_CHARACTERS_ASCII_END) :
          std::uniform_int_distribution<unsigned char>(PRINTABLE_ASCII_START, PRINTABLE_ASCII_END);

  std::string random_string;
  random_string.reserve(size);

  auto gen_random_char = [&mt, &char_dist]() { return static_cast<char>(char_dist(mt)); };
  std::generate_n(std::back_inserter(random_string), size, gen_random_char);

  return random_string;
}

//=================================================================================================
// Random vector creation
//=================================================================================================
template <typename T>
concept RandomCreatableVector = VectorType<T> && RandomCreatable<typename T::value_type>;

/// Fill a vector with randomly generated values of type T.
template <RandomCreatableVector T>
[[nodiscard]] auto random(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                          bool allow_empty = true) -> T {
  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  T vec;
  vec.reserve(size);

  auto gen = [&mt]() -> typename T::value_type { return random<typename T::value_type>(mt); };
  std::generate_n(std::back_inserter(vec), size, gen);

  return vec;
};

//=================================================================================================
// Random array creation
//=================================================================================================
template <typename T>
concept RandomCreatableArray = ArrayType<T> && RandomCreatable<typename T::value_type>;

/// Fill a vector with randomly generated values of type T.
template <RandomCreatableArray T>
[[nodiscard]] auto random(std::mt19937_64& mt) -> T {
  T array;

  std::ranges::generate(array, [&mt]() { return random<typename T::value_type>(mt); });

  return array;
};

//=================================================================================================
// Random unordered_map creation
//=================================================================================================
template <typename T>
concept RandomCreatableUnorderedMap =
    UnorderedMapType<T> && RandomCreatable<typename T::key_type> && RandomCreatable<typename T::mapped_type>;

/// Fill a vector with randomly generated values of type T.
template <RandomCreatableUnorderedMap T>
[[nodiscard]] auto random(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                          bool allow_empty = true) -> T {
  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  T umap;
  umap.reserve(size);

  auto key_gen = [&mt]() -> typename T::key_type { return random<typename T::key_type>(mt); };
  auto value_gen = [&mt]() -> typename T::mapped_type { return random<typename T::mapped_type>(mt); };
  auto gen = [&key_gen, &value_gen]() -> T::value_type { return { key_gen(), value_gen() }; };
  std::generate_n(std::inserter(umap, umap.end()), size, gen);

  return umap;
};

//=================================================================================================
// Random vector of vectors creation
//=================================================================================================
template <typename T>
concept RandomCreatableVectorOfVectors =
    VectorOfVectorsType<T> && RandomCreatableVector<typename T::value_type>;

/// Fill a vector with randomly generated values of type T.
template <RandomCreatableVectorOfVectors T>
[[nodiscard]] auto random(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                          bool allow_empty = true) -> T {
  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  T vecs;
  vecs.reserve(size);
  auto gen = [&mt]() -> typename T::value_type { return random<typename T::value_type>(mt); };
  std::generate_n(std::back_inserter(vecs), size, gen);

  return vecs;
};

//=================================================================================================
// Random vector of arrays creation
//=================================================================================================
template <typename T>
concept RandomCreatableVectorOfArrays = VectorOfArraysType<T> && RandomCreatableArray<typename T::value_type>;

/// Fill a vector with randomly generated values of type T.
template <RandomCreatableVectorOfArrays T>
[[nodiscard]] auto random(std::mt19937_64& mt, std::optional<size_t> fixed_size = std::nullopt,
                          bool allow_empty = true) -> T {
  const auto size = internal::getSize(mt, fixed_size, allow_empty);

  T vec;
  vec.reserve(size);
  auto gen = [&mt]() -> typename T::value_type { return random<typename T::value_type>(mt); };
  std::generate_n(std::back_inserter(vec), size, gen);

  return vec;
};

}  // namespace heph::random
