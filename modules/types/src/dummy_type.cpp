//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types/dummy_type.h"

#include <cstdint>
#include <random>

#include <magic_enum.hpp>

#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/types/type_formatting.h"

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

auto operator<<(std::ostream& os, const DummyPrimitivesType& dummy_primitives_type) -> std::ostream& {
  return os << "DummyPrimitivesType{" << "  dummy_bool=" << dummy_primitives_type.dummy_bool << "\n"
            << "  dummy_int8_t=" << dummy_primitives_type.dummy_int8_t << "\n"
            << "  dummy_int16_t=" << dummy_primitives_type.dummy_int16_t << "\n"
            << "  dummy_int32_t=" << dummy_primitives_type.dummy_int32_t << "\n"
            << "  dummy_int64_t=" << dummy_primitives_type.dummy_int64_t << "\n"
            << "  dummy_uint8_t=" << dummy_primitives_type.dummy_uint8_t << "\n"
            << "  dummy_uint16_t=" << dummy_primitives_type.dummy_uint16_t << "\n"
            << "  dummy_uint32_t=" << dummy_primitives_type.dummy_uint32_t << "\n"
            << "  dummy_uint64_t=" << dummy_primitives_type.dummy_uint64_t << "\n"
            << "  dummy_float=" << dummy_primitives_type.dummy_float << "\n"
            << "  dummy_double=" << dummy_primitives_type.dummy_double << "\n"
            << "}";
}

auto DummyType::random(std::mt19937_64& mt) -> DummyType {
  return { .dummy_primitives_type = random::random<decltype(dummy_primitives_type)>(mt),
           .dummy_enum = random::random<decltype(dummy_enum)>(mt),
           .dummy_timestamp_system_clock = random::random<decltype(dummy_timestamp_system_clock)>(mt),
           .dummy_timestamp_steady_clock = random::random<decltype(dummy_timestamp_steady_clock)>(mt),
           .dummy_string = random::random<decltype(dummy_string)>(mt),
           .dummy_vector = random::random<decltype(dummy_vector)>(mt) };
}

auto operator<<(std::ostream& os, const DummyType& dummy_type) -> std::ostream& {
  return os << "DummyType{" << "  dummy_primitives_type={" << dummy_type.dummy_primitives_type << "}\n"
            << "  dummy_enum=" << magic_enum::enum_name(dummy_type.dummy_enum) << "\n"
            << "  dummy_timestamp_system_clock=" << toString(dummy_type.dummy_timestamp_system_clock) << "\n"
            << "  dummy_timestamp_steady_clock=" << toString(dummy_type.dummy_timestamp_steady_clock) << "\n"
            << "  dummy_string=" << dummy_type.dummy_string << "\n"
            << "  dummy_vector=" << toString(dummy_type.dummy_vector) << "\n"
            << "}";
}

}  // namespace heph::types
