//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace heph::types {

/// @brief Collection of primitive types for testing purposes.
/// NOTE: the data needs to be Protobuf serializable
/// NOTE: missing primitive types shall be added at any time to increase the test coverage
struct DummyPrimitivesType {
  [[nodiscard]] auto operator==(const DummyPrimitivesType&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> DummyPrimitivesType;

  bool dummy_bool{};

  int8_t dummy_int8_t{};
  int16_t dummy_int16_t{};
  int32_t dummy_int32_t{};
  int64_t dummy_int64_t{};

  uint8_t dummy_uint8_t{};
  uint16_t dummy_uint16_t{};
  uint32_t dummy_uint32_t{};
  uint64_t dummy_uint64_t{};

  float dummy_float{};
  double dummy_double{};
};

enum class ExternalDummyEnum : int8_t { A, B, C, D, E, F, G };

/// @brief Collection of non-primitive types for testing purposes.
/// NOTE: the data needs to be Protobuf serializable
/// NOTE: missing generic non-primitive types can be added to increase the test coverage
struct DummyType {
  enum class InternalDummyEnum : int8_t { ALPHA };

  [[nodiscard]] auto operator==(const DummyType&) const -> bool = default;

  [[nodiscard]] static auto random(std::mt19937_64& mt) -> DummyType;

  DummyPrimitivesType dummy_primitives_type{};

  InternalDummyEnum internal_dummy_enum{};
  ExternalDummyEnum external_dummy_enum{};

  std::string dummy_string{};
  std::vector<int32_t> dummy_vector{};
  std::vector<DummyPrimitivesType> dummy_vector_encapsulated{};
};

}  // namespace heph::types
