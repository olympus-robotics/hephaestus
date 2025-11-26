//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

#include <fmt/format.h>
#include <magic_enum.hpp>

#include "hephaestus/error_handling/panic.h"
#include "hephaestus/utils/concepts.h"
#include "hephaestus/utils/format/format.h"
#include "hephaestus/utils/string/string_utils.h"

namespace heph::serdes::protobuf {

template <EnumType ProtoT, EnumType T>
[[nodiscard]] auto toProtoEnum(const T& enum_value) -> ProtoT;

template <EnumType ProtoT, EnumType T>
void fromProto(const ProtoT& proto_enum_value, T& enum_value);

//=================================================================================================
// Implementation
//=================================================================================================

namespace internal {
template <EnumType ProtoT>
[[nodiscard]] auto getProtoPrefix() -> std::string {
  const auto enum_type_name = magic_enum::enum_type_name<ProtoT>();

  // Underscores indicate nested proto enums, no underscore indicates a top-level proto enum.
  auto underscore_pos = std::find(enum_type_name.begin(), enum_type_name.end(), '_');
  if (underscore_pos == enum_type_name.end()) {
    // Top-level enums use the enum name as a prefix: ENUM_NAME_ENUM_VALUE.
    return utils::string::toScreamingSnakeCase(enum_type_name);
  }

  // Nested enums have a prefix, and enum values are separated by an underscore:
  // ClassName_EnumName_ENUM_NAME_ENUM_VALUE.
  const auto internal_enum_type_name = std::string_view{ ++underscore_pos, enum_type_name.end() };
  return fmt::format("{}_{}", enum_type_name, utils::string::toScreamingSnakeCase(internal_enum_type_name));
}

/// Convert between enums and their protobuf counterparts. The following convention is used:
/// enum class FooExternalEnum : { BAR1, BAR2 };
/// struct Foo {
///   enum class InternalEnum : { BAR1, BAR2 };
/// };
/// will have a protobuf counterpart
/// enum FooExternalEnum : { FOO_EXTERNAL_ENUM_BAR1,  FOO_EXTERNAL_ENUM_BAR2 };
/// enum Foo_InternalEnum : { Foo_InternalEnum_BAR1, Foo_InternalEnum_BAR2 };
template <EnumType ProtoT, EnumType T>
[[nodiscard]] auto getAsProtoEnum(T e) -> ProtoT {
  const auto proto_enum_name = fmt::format("{}_{}", getProtoPrefix<ProtoT>(), magic_enum::enum_name(e));
  const auto proto_enum = magic_enum::enum_cast<ProtoT>(proto_enum_name);

  HEPH_PANIC_IF(!proto_enum.has_value(),
                "The proto enum does not contain the requested key {}. Proto enum keys are\n{}",
                proto_enum_name, utils::format::toString(magic_enum::enum_names<ProtoT>()));

  return proto_enum.value();  // NOLINT(bugprone-unchecked-optional-access)
}

template <EnumType ProtoT, EnumType T>
[[nodiscard]] auto createEnumLookupTable() -> std::unordered_map<T, ProtoT> {
  std::unordered_map<T, ProtoT> lookup_table;

  for (const auto& e : magic_enum::enum_values<T>()) {
    lookup_table[e] = getAsProtoEnum<ProtoT, T>(e);
  }

  return lookup_table;
}

/// @brief Create inverse lookup table. Unique values are guaranteed by the enum layout.
template <EnumType ProtoT, EnumType T>
[[nodiscard]] auto createInverseLookupTable(const std::unordered_map<T, ProtoT>& enum_to_proto_enum)
    -> std::unordered_map<ProtoT, T> {
  std::unordered_map<ProtoT, T> proto_enum_to_enum;

  for (const auto& kvp : enum_to_proto_enum) {
    proto_enum_to_enum.insert({ kvp.second, kvp.first });
  }

  return proto_enum_to_enum;
}
}  // namespace internal

template <EnumType ProtoT, EnumType T>
auto toProtoEnum(const T& enum_value) -> ProtoT {
  static const auto enum_to_proto_enum = internal::createEnumLookupTable<ProtoT, T>();
  const auto it = enum_to_proto_enum.find(enum_value);
  HEPH_PANIC_IF(it == enum_to_proto_enum.end(), "Enum {} not found in the lookup table",
                utils::format::toString(enum_value));
  return it->second;
}

template <EnumType ProtoT, EnumType T>
void fromProto(const ProtoT& proto_enum_value, T& enum_value) {
  static const auto proto_enum_value_to_enum =
      internal::createInverseLookupTable(internal::createEnumLookupTable<ProtoT, T>());
  const auto it = proto_enum_value_to_enum.find(proto_enum_value);
  HEPH_PANIC_IF(it == proto_enum_value_to_enum.end(), "Enum {} not found in the lookup table",
                utils::format::toString(proto_enum_value));
  enum_value = it->second;
  ;
}

}  // namespace heph::serdes::protobuf
