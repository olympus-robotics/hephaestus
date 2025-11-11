//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>

#include <rfl/to_view.hpp>

#include "hephaestus/telemetry/metric_sink.h"
#include "hephaestus/utils/concepts.h"

namespace heph::telemetry::detail {
template <typename T>
auto toValue(const T& val) -> Metric::ValueType {
  if constexpr (std::is_same_v<T, bool>) {
    return val;
  } else if constexpr (std::is_integral_v<T>) {
    return static_cast<int64_t>(val);
  } else if constexpr (std::is_floating_point_v<T>) {
    return static_cast<double>(val);
  } else if constexpr (std::is_convertible_v<T, std::string>) {
    return std::string(val);
  } else {
    static_assert(sizeof(T) == 0, "Unsupported type for Metric::ValueType conversion");
  }
}

template <typename T>
constexpr auto isPrimitive() -> bool {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  return std::is_integral_v<U> || std::is_floating_point_v<U> || std::is_same_v<U, std::string> ||
         std::is_same_v<U, bool>;
}

// Forward declaration
template <typename T>
void toMapImpl(const T& data, const std::string& prefix,
               std::unordered_map<std::string, Metric::ValueType>& result);

// Process a single field
template <typename T>
void processField(const T& field, const std::string& name,
                  std::unordered_map<std::string, Metric::ValueType>& result) {
  static_assert(!VectorType<T> && !ArrayType<T> && !OptionalType<T>,
                "Vectors, arrays and optionals are not supported in metrics");
  if constexpr (isPrimitive<T>()) {
    result[name] = toValue(field);
  } else {
    // Recursively process nested struct
    toMapImpl(field, name, result);
  }
}

// Main implementation using reflect-cpp
template <typename T>
void toMapImpl(const T& data, const std::string& prefix,
               std::unordered_map<std::string, Metric::ValueType>& result) {
  const auto view = rfl::to_view(data);

  view.apply([&](const auto& field) {
    const auto field_name = std::string(field.name());
    const auto full_name = prefix.empty() ? field_name : prefix + "." + field_name;
    const auto& value = *(field.value());
    processField(value, full_name, result);
  });
}

template <typename T>
auto structToFlatMap(const T& data) -> std::unordered_map<std::string, Metric::ValueType> {
  std::unordered_map<std::string, Metric::ValueType> result;
  toMapImpl(data, "", result);
  return result;
}

}  // namespace heph::telemetry::detail
