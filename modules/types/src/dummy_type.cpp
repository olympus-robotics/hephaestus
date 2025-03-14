//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types/dummy_type.h"

#include <cstdint>
#include <random>

#include "hephaestus/random/random_object_creator.h"

namespace heph::types {

auto DummyPrimitivesType::random(std::mt19937_64& mt) -> DummyPrimitivesType {
  return { .dummy_bool = random::random<bool>(mt),
           .dummy_int8_t = random::random<int8_t>(mt),
           .dummy_int16_t = random::random<int16_t>(mt),
           .dummy_int32_t = random::random<int32_t>(mt),
           .dummy_int64_t = random::random<int64_t>(mt),
           .dummy_uint8_t = random::random<uint8_t>(mt),
           .dummy_uint16_t = random::random<uint16_t>(mt),
           .dummy_uint32_t = random::random<uint32_t>(mt),
           .dummy_uint64_t = random::random<uint64_t>(mt),
           .dummy_float = random::random<float>(mt),
           .dummy_double = random::random<double>(mt) };
}

auto DummyType::random(std::mt19937_64& mt) -> DummyType {
  return { .dummy_primitives_type = random::random<decltype(dummy_primitives_type)>(mt),
           .internal_dummy_enum = random::random<decltype(internal_dummy_enum)>(mt),
           .external_dummy_enum = random::random<decltype(external_dummy_enum)>(mt),
           .dummy_string = random::random<decltype(dummy_string)>(mt),
           .dummy_vector = random::random<decltype(dummy_vector)>(mt) };
}

}  // namespace heph::types
