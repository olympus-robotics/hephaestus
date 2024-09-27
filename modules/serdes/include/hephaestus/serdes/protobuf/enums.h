//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string_view>
#include <type_traits>
#include <unordered_map>

#include <fmt/core.h>
#include <magic_enum.hpp>

#include "hephaestus/utils/concepts.h"
#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/format/format.h"
#include "hephaestus/utils/string/string_literal.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::serdes::protobuf {

template <EnumType T, EnumType ProtoT>
[[nodiscard]] auto toProtoEnum(const T& enum_value) -> ProtoT;

template <EnumType T, EnumType ProtoT>
auto fromProto(const ProtoT& proto_enum_value, T& enum_value) -> void;

//=================================================================================================
// Implementation
//=================================================================================================

namespace internal {
template <EnumType ProtoT>
[[nodiscard]] auto getProtoPrefix() -> std::string {
  const auto enum_type_name = magic_enum::enum_type_name<ProtoT>();

  // Underscores indicate nested proto enums, no underscore means it's a top-level proto enum. Top level enums
  // do not have a prefix.
  if (const auto underscore_pos = std::find(enum_type_name.begin(), enum_type_name.end(), '_');
      underscore_pos == enum_type_name.end()) {
    return "";
  }

  // Nested enums have a prefix, and enum values are separated by an underscore:
  // ClassName_EnumName_ENUM_VALUE.
  return fmt::format("{}_", enum_type_name);
}

/// Convert between enums and their protobuf counterparts. The following convention is used:
/// enum class FooExternalEnum : { BAR1, BAR2 };
/// struct Foo {
///   enum class InternalEnum : { BAR1, BAR2 };
/// };
/// will have a protobuf counterpart
/// enum FooExternalEnum : { BAR1,  BAR2 };
/// enum Foo_InternalEnum : { Foo_InternalEnum_BAR1, Foo_InternalEnum_BAR2 };
template <EnumType T, EnumType ProtoT>
[[nodiscard]] auto getAsProtoEnum(T e) -> ProtoT {
  const auto proto_prefix = getProtoPrefix<ProtoT>();
  auto proto_enum_name =
      fmt::format("{}{}", proto_prefix, magic_enum::enum_name(e));  // ClassName_EnumName_ENUM_VALUE

  auto proto_enum = magic_enum::enum_cast<ProtoT>(proto_enum_name);
  heph::throwExceptionIf<heph::InvalidParameterException>(
      !proto_enum.has_value(),
      fmt::format("The proto enum does not contain the requested key {}. Proto enum keys are\n{}",
                  proto_enum_name, utils::format::toString(magic_enum::enum_names<ProtoT>())));

  return proto_enum.value();
}

template <EnumType T, EnumType ProtoT>
[[nodiscard]] auto createEnumLookupTable() -> std::unordered_map<T, ProtoT> {
  std::unordered_map<T, ProtoT> lookup_table;

  for (const auto& e : magic_enum::enum_values<T>()) {
    lookup_table[e] = getAsProtoEnum<T, ProtoT>(e);
  }

  return lookup_table;
}

/// @brief Create inverse lookup table. Unique values are guaranteed by the enum layout.
template <EnumType T, EnumType ProtoT>
[[nodiscard]] auto createInverseLookupTable(const std::unordered_map<T, ProtoT>& enum_to_proto_enum)
    -> std::unordered_map<ProtoT, T> {
  std::unordered_map<ProtoT, T> proto_enum_to_enum;

  for (const auto& kvp : enum_to_proto_enum) {
    proto_enum_to_enum.insert({ kvp.second, kvp.first });
  }

  return proto_enum_to_enum;
}
}  // namespace internal

template <EnumType T, EnumType ProtoT>
auto toProtoEnum(const T& enum_value) -> ProtoT {
  static const auto enum_to_proto_enum = internal::createEnumLookupTable<T, ProtoT>();
  try {
    return enum_to_proto_enum.at(enum_value);
  } catch (const std::out_of_range&) {
    throw std::out_of_range(
        fmt::format("Enum {} not found in the lookup table", magic_enum::enum_name(enum_value)));
  }
}

template <EnumType T, EnumType ProtoT>
auto fromProto(const ProtoT& proto_enum_value, T& enum_value) -> void {
  static const auto proto_enum_value_to_enum =
      internal::createInverseLookupTable(internal::createEnumLookupTable<T, ProtoT>());
  try {
    enum_value = proto_enum_value_to_enum.at(proto_enum_value);
  } catch (const std::out_of_range&) {
    throw std::out_of_range(
        fmt::format("Proto enum {} not found in the lookup table", magic_enum::enum_name(proto_enum_value)));
  }
}

}  // namespace heph::serdes::protobuf
