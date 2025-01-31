//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/string/string_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>

#include <absl/strings/ascii.h>

namespace heph::utils::string {

auto toSnakeCase(std::string_view camel_case) -> std::string {
  std::string snake_case;
  snake_case.reserve(camel_case.size() * 2);  // Reserve enough space to avoid reallocations

  for (std::size_t i = 0; i < camel_case.size(); ++i) {
    // Add an underscore if it's an uppercase letter not following another uppercase letter.
    if ((i != 0)                                      // Omit first letter to avoid leading underscore.
        && (std::isupper(camel_case[i]) != 0)         // Current letter is uppercase AND
        && (std::islower(camel_case[i - 1]) != 0)) {  // previous letter was lowercase.
      snake_case.push_back('_');
    }

    snake_case.push_back(static_cast<char>(std::tolower(static_cast<int>(camel_case[i]))));
  }

  return snake_case;
}

auto toScreamingSnakeCase(std::string_view camel_case) -> std::string {
  auto snake_case = toSnakeCase(camel_case);
  return absl::AsciiStrToUpper(snake_case);
}

auto toAsciiHex(const std::string& input) -> std::string {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');

  for (const char c : input) {
    ss << std::setw(2) << static_cast<int>(c);
  }

  return ss.str();
}

auto isAlphanumericString(const std::string& input) -> bool {
  return std::ranges::all_of(input, [](char c) { return std::isalnum(c) != 0; });
}

void removeNonAlphanumericChar(std::string& str) {
  const auto [first, last] = std::ranges::remove_if(str, [](char c) { return !std::isalnum(c); });
  str.erase(first, last);
}

}  // namespace heph::utils::string
