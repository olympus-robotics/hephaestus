//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>

namespace heph::utils::string {

/// Truncates a string by returning the segment between the first start token and the first end token strings,
/// including the start token and the end token.
/// If \param include_end_token is false, the end token will not be included in the result.
/// For example, the following code will return 'to/some/file'
/// @code
/// constexpr auto str = "/path/to/some/file.txt";
/// constexpr auto start_token = "to";
/// constexpr auto end_token = ".txt";
/// constexpr auto truncated = truncate(str, start_token, end_token, false);
/// std::cout << truncated << '\n'; // $ to/some/file
/// @endcode
[[nodiscard]] constexpr auto truncate(std::string_view str, std::string_view start_token,
                                      std::string_view end_token = std::string_view(""),
                                      bool include_end_token = true) -> std::string_view;

/// aNy_CaSe -> ANY_CASE
[[nodiscard]] auto toUpperCase(const std::string_view& any_case) -> std::string;

/// camelCase -> camel_case
[[nodiscard]] auto toSnakeCase(const std::string_view& camel_case) -> std::string;

/// camelCase -> CAMEL_CASE
[[nodiscard]] auto toScreamingSnakeCase(const std::string_view& camel_case) -> std::string;

[[nodiscard]] auto stringToInt64(const std::string& str) -> std::optional<int64_t>;

//=================================================================================================
// Implementation
//=================================================================================================

constexpr auto truncate(std::string_view str, std::string_view start_token, std::string_view end_token,
                        bool include_end_token) -> std::string_view {
  const auto start_pos = str.find(start_token);
  auto end_pos = end_token.empty() ? std::string_view::npos : str.find(end_token);
  if (end_pos != std::string_view::npos && include_end_token) {
    end_pos += end_token.size();
  }

  return (start_pos != std::string_view::npos) ? str.substr(start_pos, end_pos - start_pos) :
                                                 str.substr(0, end_pos);
}

template <typename T>
  requires std::is_same_v<T, int64_t> || std::is_same_v<T, double>
[[nodiscard]] auto stringTo(const std::string& str) -> std::optional<T> {
  const char* start = str.c_str();
  char* end{};
  errno = 0;
  T result{};
  if constexpr (std::is_same_v<T, int64_t>) {
    static constexpr int BASE = 10;
    result = std::strtoll(start, &end, BASE);
  } else {
    result = std::strtod(start, &end);
  }

  if (errno != ERANGE && *end == '\0') {
    return result;
  }

  return std::nullopt;
}

}  // namespace heph::utils::string
