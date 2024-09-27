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
#include "hephaestus/utils/string/string_literal.h"
#include "hephaestus/utils/string/string_utils.h"
#include "hephaestus/utils/string/type_formatting.h"

namespace heph::serdes::protobuf {

/// Convert between enums and their protobuf counterparts using a lookup table. A singleton pattern with lazy
/// evaluation of the lookup table is used. For a given pair of enums and proto enums, the converter is unique
/// and only created once. If you need a challenge, convert the class to constexpr and evaluate at compile
/// time.
template <EnumType T, EnumType ProtoT, utils::string::StringLiteral ProtoPrefix>
class EnumProtoConverter {
public:
  EnumProtoConverter(const EnumProtoConverter&) = delete;
  auto operator=(const EnumProtoConverter&) -> EnumProtoConverter& = delete;
  EnumProtoConverter(EnumProtoConverter&&) = delete;
  auto operator=(EnumProtoConverter&&) -> EnumProtoConverter& = delete;

  [[nodiscard]] static auto toProtoEnum(const T& e) -> ProtoT;
  [[nodiscard]] static auto fromProtoEnum(const ProtoT& proto_enum) -> T;

private:
  EnumProtoConverter();
  ~EnumProtoConverter() = default;

  [[nodiscard]] static auto instance() -> const EnumProtoConverter<T, ProtoT, ProtoPrefix>&;
  [[nodiscard]] static auto getAsProtoEnum(T e) -> ProtoT;
  [[nodiscard]] static auto createEnumLookupTable() -> std::unordered_map<T, ProtoT>;

  std::unordered_map<T, ProtoT> enum_to_proto_enum_;  // Lookup table.
  std::unordered_map<ProtoT, T> proto_enum_to_enum_;  // Invese lookup table.
};

//=================================================================================================
// Implementation
//=================================================================================================

template <EnumType T, EnumType ProtoT, utils::string::StringLiteral ProtoPrefix>
auto EnumProtoConverter<T, ProtoT, ProtoPrefix>::toProtoEnum(const T& e) -> ProtoT {
  static const auto& converter = instance();
  return converter.enum_to_proto_enum_.at(e);
}

template <EnumType T, EnumType ProtoT, utils::string::StringLiteral ProtoPrefix>
auto EnumProtoConverter<T, ProtoT, ProtoPrefix>::fromProtoEnum(const ProtoT& proto_enum) -> T {
  static const auto& converter = instance();
  return converter.proto_enum_to_enum_.at(proto_enum);
}

/// Convert between enums and their protobuf counterparts. The following convention is used:
/// enum class FooExternalEnum : { BAR1, BAR2 };
/// struct Foo {
///   enum class InternalEnum : { BAR1, BAR2 };
/// };
/// will have a protobuf counterpart
/// enum FOO_EXTERNAL_ENUM : {  FOO_EXTERNAL_ENUM_BAR1,  FOO_INTERNAL_ENUM_BAR2 };
/// enum FOO_INTERNAL_ENUM : {  FOO_INTERNAL_ENUM_BAR1,  FOO_INTERNAL_ENUM_BAR2 };
template <EnumType T, EnumType ProtoT, utils::string::StringLiteral ProtoPrefix>
auto EnumProtoConverter<T, ProtoT, ProtoPrefix>::getAsProtoEnum(T e) -> ProtoT {
  const auto& enum_name = magic_enum::enum_name(e);
  auto proto_prefix = static_cast<std::string_view>(ProtoPrefix);
  auto proto_enum_name = fmt::format("{}{}{}", proto_prefix, proto_prefix.empty() ? "" : "_", enum_name);

  auto proto_enum = magic_enum::enum_cast<ProtoT>(proto_enum_name);
  heph::throwExceptionIf<heph::InvalidParameterException>(
      !proto_enum.has_value(),
      fmt::format("The proto enum does not contain the requested key {}. Proto enum keys are {}",
                  proto_enum_name, utils::string::toString(magic_enum::enum_names<ProtoT>())));

  return proto_enum.value();
}

template <EnumType T, EnumType ProtoT, utils::string::StringLiteral ProtoPrefix>
auto EnumProtoConverter<T, ProtoT, ProtoPrefix>::createEnumLookupTable() -> std::unordered_map<T, ProtoT> {
  std::unordered_map<T, ProtoT> lookup_table;

  for (const auto& e : magic_enum::enum_values<T>()) {
    lookup_table[e] = EnumProtoConverter<T, ProtoT, ProtoPrefix>::getAsProtoEnum(e);
  }

  return lookup_table;
}

template <EnumType T, EnumType ProtoT, utils::string::StringLiteral ProtoPrefix>
EnumProtoConverter<T, ProtoT, ProtoPrefix>::EnumProtoConverter()
  : enum_to_proto_enum_{ createEnumLookupTable() } {
  // Create inverse lookup table. Unique values are guaranteed by the enum.
  for (const auto& kvp : enum_to_proto_enum_) {
    proto_enum_to_enum_.insert({ kvp.second, kvp.first });
  }
  for (const auto& kvp : enum_to_proto_enum_) {
    fmt::println("{} : {}", magic_enum::enum_name(kvp.first), magic_enum::enum_name(kvp.second));
  }
}

template <EnumType T, EnumType ProtoT, utils::string::StringLiteral ProtoPrefix>
auto EnumProtoConverter<T, ProtoT, ProtoPrefix>::instance()
    -> const EnumProtoConverter<T, ProtoT, ProtoPrefix>& {
  static EnumProtoConverter<T, ProtoT, ProtoPrefix> instance;
  return instance;
}

}  // namespace heph::serdes::protobuf
