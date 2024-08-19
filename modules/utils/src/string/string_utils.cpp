//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/string/string_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>

namespace heph::utils::string {

auto toUpperCase(std::string_view any_case) -> std::string {
  std::string upper_case;
  upper_case.reserve(any_case.size());
  std::transform(any_case.begin(), any_case.end(), std::back_inserter(upper_case),
                 [](unsigned char c) { return std::toupper(c); });

  return upper_case;
}

auto toLowerCase(std::string_view any_case) -> std::string {
  std::string lower_case;
  lower_case.reserve(any_case.size());
  std::transform(any_case.begin(), any_case.end(), std::back_inserter(lower_case),
                 [](unsigned char c) { return std::tolower(c); });

  return lower_case;
}

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
  return toUpperCase(snake_case);
}

}  // namespace heph::utils::string
