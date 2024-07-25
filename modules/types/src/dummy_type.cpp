//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/types/dummy_type.h"

#include <random>

#include "hephaestus/random/random_container.h"
#include "hephaestus/random/random_type.h"
#include "hephaestus/types/type_formatting.h"

namespace heph::types {

auto DummyPrimitivesType::random(std::mt19937_64& mt) -> DummyPrimitivesType {
  return { .dummy_bool = heph::random::randomT<bool>(mt),
           .dummy_int8_t = heph::random::randomT<int8_t>(mt),
           .dummy_int16_t = heph::random::randomT<int16_t>(mt),
           .dummy_int32_t = heph::random::randomT<int32_t>(mt),
           .dummy_int64_t = heph::random::randomT<int64_t>(mt),
           .dummy_uint8_t = heph::random::randomT<uint8_t>(mt),
           .dummy_uint16_t = heph::random::randomT<uint16_t>(mt),
           .dummy_uint32_t = heph::random::randomT<uint32_t>(mt),
           .dummy_uint64_t = heph::random::randomT<uint64_t>(mt),
           .dummy_float = heph::random::randomT<float>(mt),
           .dummy_double = heph::random::randomT<double>(mt) };
}

auto operator<<(std::ostream& os, const DummyPrimitivesType& dummy_primitives_type) -> std::ostream& {
  return os << "DummyPrimitivesType{"
            << "dummy_bool=" << dummy_primitives_type.dummy_bool << "\n"
            << "dummy_int8_t=" << dummy_primitives_type.dummy_int8_t << "\n"
            << "dummy_int16_t=" << dummy_primitives_type.dummy_int16_t << "\n"
            << "dummy_int32_t=" << dummy_primitives_type.dummy_int32_t << "\n"
            << "dummy_int64_t=" << dummy_primitives_type.dummy_int64_t << "\n"
            << "dummy_uint8_t=" << dummy_primitives_type.dummy_uint8_t << "\n"
            << "dummy_uint16_t=" << dummy_primitives_type.dummy_uint16_t << "\n"
            << "dummy_uint32_t=" << dummy_primitives_type.dummy_uint32_t << "\n"
            << "dummy_uint64_t=" << dummy_primitives_type.dummy_uint64_t << "\n"
            << "dummy_float=" << dummy_primitives_type.dummy_float << "\n"
            << "dummy_double=" << dummy_primitives_type.dummy_double << "\n"
            << "}";
}

auto DummyType::random(std::mt19937_64& mt) -> DummyType {
  return { .dummy_primitives_type = heph::random::randomT<DummyPrimitivesType>(mt),
           .dummy_enum = heph::random::randomT<DummyEnum>(mt),
           .dummy_timestamp = heph::random::randomT<TimestampT>(mt),
           .dummy_string = heph::random::randomT<std::string>(mt),
           .dummy_vector = heph::random::randomT<std::vector<int>>(mt) };
}

auto operator<<(std::ostream& os, const DummyType& dummy_type) -> std::ostream& {
  return os << "DummyType{"
            << "  dummy_primitives_type={" << dummy_type.dummy_primitives_type << "}\n"
            << "  dummy_enum=" << magic_enum::enum_name(dummy_type.dummy_enum) << "\n"
            << "  dummy_timestamp=" << toString(dummy_type.dummy_timestamp) << "\n"
            << "  dummy_string=" << dummy_type.dummy_string << "\n"
            << "  dummy_vector=" << toString(dummy_type.dummy_vector) << "\n"
            << "}";
}

}  // namespace heph::types
