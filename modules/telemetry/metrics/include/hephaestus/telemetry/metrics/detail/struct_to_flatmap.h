//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include <rfl/to_view.hpp>

#include "hephaestus/telemetry/metrics/metric_sink.h"
#include "hephaestus/utils/concepts.h"

namespace heph::telemetry::detail {
namespace impl {
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
  } else if constexpr (EnumType<T>) {
    return static_cast<int64_t>(val);
  } else if constexpr (ChronoTimestampType<T>) {
    return val.time_since_epoch().count();
  } else {
    static_assert(sizeof(T) == 0, "Unsupported type for Metric::ValueType conversion");
  }
}

template <typename T>
constexpr auto isSupportedType() -> bool {
  using U = std::remove_cv_t<std::remove_reference_t<T>>;
  return std::is_integral_v<U> || std::is_floating_point_v<U> || std::is_same_v<U, std::string> ||
         std::is_same_v<U, bool> || EnumType<U> || ChronoTimestampType<U>;
}

template <typename T>
void structToKeyValuePairs(const T& data, const std::string& prefix,
                           std::vector<Metric::KeyValueType>& result);

template <typename T>
void processField(const T& field, const std::string& name, std::vector<Metric::KeyValueType>& result) {
  static_assert(!VectorType<T> && !ArrayType<T> && !OptionalType<T>,
                "Vectors, arrays and optionals are not supported in metrics");
  if constexpr (isSupportedType<T>()) {
    // End of recursion
    result.emplace_back(name, toValue(field));
  } else {
    // Recursively process nested struct
    structToKeyValuePairs(field, name, result);
  }
}

template <typename T>
void structToKeyValuePairs(const T& data, const std::string& prefix,
                           std::vector<Metric::KeyValueType>& result) {
  const auto view = rfl::to_view(data);

  view.apply([&](const auto& field) {
    const auto field_name = std::string(field.name());
    const auto full_name = prefix.empty() ? field_name : prefix + "." + field_name;
    const auto& value = *(field.value());
    processField(value, full_name, result);
  });
}
}  // namespace impl

template <typename T>
auto structToKeyValuePairs(const T& data) -> std::vector<Metric::KeyValueType> {
  std::vector<Metric::KeyValueType> result;
  impl::structToKeyValuePairs(data, "", result);
  return result;
}

}  // namespace heph::telemetry::detail
