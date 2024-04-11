//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/utils/string/string_utils.h"

#include <algorithm>

namespace heph::utils::string {

auto toUpperCase(const std::string_view& any_case) -> std::string {
  std::string upper_case;
  upper_case.reserve(any_case.size());
  std::transform(any_case.begin(), any_case.end(), std::back_inserter(upper_case),
                 [](unsigned char c) { return std::toupper(c); });

  return upper_case;
}

auto toSnakeCase(const std::string_view& camel_case) -> std::string {
  std::string snake_case;
  snake_case.reserve(camel_case.size() * 2);  // Reserve enough space to avoid reallocations

  for (size_t i = 0; i < camel_case.size(); ++i) {
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

auto toScreamingSnakeCase(const std::string_view& camel_case) -> std::string {
  auto snake_case = toSnakeCase(camel_case);
  return toUpperCase(snake_case);
}

}  // namespace heph::utils::string
