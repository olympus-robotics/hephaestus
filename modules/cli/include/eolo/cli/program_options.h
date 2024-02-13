//=================================================================================================
// Copyright(C) 2018 GRAPE Contributors
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#pragma once

#include <algorithm>
#include <format>
#include <sstream>
#include <string>
#include <vector>

#include "eolo/base/exception.h"
#include "eolo/utils/concepts.h"
#include "eolo/utils/utils.h"

namespace eolo::cli {

//=================================================================================================
/// Container for command line options for a program. To be used with ProgramDescription class.
class ProgramOptions {
public:
  /// @brief  Holds program options and their details
  struct Option {
    std::string key;
    char short_key{};
    std::string description;
    std::string value_type;
    std::string value;
    bool is_required{ false };
    bool is_specified{ false };
  };

public:
  /// Check whether an option was specified on the command line
  /// @param option  The command line option (without the '--').
  /// @return true if option is found
  [[nodiscard]] auto hasOption(const std::string& option) const -> bool;

  /// Get the value specified for a command line option
  /// @param option  The command line option (without the '--').
  /// @return  The value of the specified option.
  template <StringStreamable T>
  [[nodiscard]] auto getOption(const std::string& option) const -> T;

private:
  friend class ProgramDescription;

  /// @brief Constructor. See ProgramDescription::parse
  /// @param options List of supported program options
  explicit ProgramOptions(std::vector<Option>&& options);

  std::vector<Option> options_;
};

//=================================================================================================
/// Program description and command line parsing utility. Works with ProgramOptions class.
///
/// Features:
/// - Enforces that every supported command line option is described exactly once
/// - Throws if unsupported options are specified on the command line
/// - Throws if required options are not specified on the command line
/// - Throws if value types are mismatched between where they are defined and where they are used
/// - Ensures 'help' is always available
/// See also ProgramOptions
class ProgramDescription {
public:
  /// @brief Creates object
  /// @param brief A brief text describing the program
  constexpr explicit ProgramDescription(std::string brief);

  /// @brief Defines a required option (--key=value) on the command line
  /// @tparam T Value type
  /// @param key Key of the key-value pair
  /// @param description A brief text describing the option
  /// @return Reference to the object. Enables daisy-chained calls
  template <StringStreamable T>
  constexpr auto defineOption(const std::string& key, const std::string& description) -> ProgramDescription&;

  /// @brief Defines a required option (--key=value) on the command line
  /// @tparam T Value type
  /// @param key Key of the key-value pair
  /// @param short_key Single char (can be used as alias for --key)
  /// @param description A brief text describing the option
  /// @return Reference to the object. Enables daisy-chained calls
  template <StringStreamable T>
  constexpr auto defineOption(const std::string& key, char short_key,
                              const std::string& description) -> ProgramDescription&;

  /// @brief Defines a command line option (--key=value) that is optional
  /// @tparam T Value type
  /// @param key Key of the key-value pair
  /// @param description A brief text describing the option
  /// @param default_value Default value to use if the option is not specified on the command line
  /// @return Reference to the object. Enables daisy-chained calls
  template <StringStreamable T>
  constexpr auto defineOption(const std::string& key, const std::string& description,
                              const T& default_value) -> ProgramDescription&;

  /// @brief Defines a command line option (--key=value) that is optional
  /// @tparam T Value type
  /// @param key Key of the key-value pair
  /// @param short_key Single char (can be used as alias for --key)
  /// @param description A brief text describing the option
  /// @param default_value Default value to use if the option is not specified on the command line
  /// @return Reference to the object. Enables daisy-chained calls
  template <StringStreamable T>
  constexpr auto defineOption(const std::string& key, char short_key, const std::string& description,
                              const T& default_value) -> ProgramDescription&;

  /// @brief Defines a boolean option (flag) (--key=value) on the command line. If the flag is
  /// passed the value of the option is true, false otherwise.
  /// @param key Key of the key-value pair
  /// @param short_key Single char (can be used as alias for --key)
  /// @param description A brief text describing the option
  /// @return Reference to the object. Enables daisy-chained calls
  constexpr auto defineFlag(const std::string& key, char short_key,
                            const std::string& description) -> ProgramDescription&;

  /// @brief Defines a boolean option (flag) (--key=value) on the command line. If the flag is
  /// passed the value of the option is true, false otherwise.
  /// @param key Key of the key-value pair
  /// @param description A brief text describing the option
  /// @return Reference to the object. Enables daisy-chained calls
  constexpr auto defineFlag(const std::string& key, const std::string& description) -> ProgramDescription&;

  /// @brief Builds the container to parse command line options.
  /// @note: The resources in this object is moved into the returned object, making this object
  /// unvalid.
  /// @param argc Number of arguments on the command line
  /// @param argv array of C-style strings
  /// @return Object containing command line options
  auto parse(int argc, const char** argv) && -> ProgramOptions;

  /// @brief Builds the container to parse command line options.
  /// @note: The resources in this object is moved into the returned object, making this object
  /// unvalid.
  /// @param args List of element passed to the program
  /// @return Object containing command line options
  auto parse(const std::vector<std::string>& args) && -> ProgramOptions;

private:
  void checkOptionAlreadyExists(const std::string& key, char k) const;

  [[nodiscard]] auto getOptionFromArg(const std::string& arg) -> ProgramOptions::Option&;

  [[nodiscard]] auto getHelpMessage() const -> std::string;

private:
  static constexpr auto HELP_KEY = "help";
  static constexpr char HELP_SHORT_KEY = 'h';
  std::string brief_;
  std::vector<ProgramOptions::Option> options_;
};

constexpr ProgramDescription::ProgramDescription(std::string brief) : brief_(std::move(brief)) {
  options_.emplace_back(HELP_KEY, HELP_SHORT_KEY, "", utils::getTypeName<std::string>(), "", false, false);
}

void ProgramDescription::checkOptionAlreadyExists(const std::string& key, char k) const {
  const auto it =
      std::find_if(options_.begin(), options_.end(), [&key](const auto& opt) { return key == opt.key; });
  throwExceptionIf<InvalidOperationException>(it != options_.end(),
                                              std::format("Attempted redefinition of option '{}'", key));

  if (k == '\0') {
    return;
  }

  const auto short_it =
      std::find_if(options_.begin(), options_.end(), [k](const auto& opt) { return k == opt.short_key; });
  throwExceptionIf<InvalidOperationException>(
      short_it != options_.end(),
      std::format("Attempted redefinition of short key '{}' for option '{}'", k, key));
}

template <StringStreamable T>
constexpr auto ProgramDescription::defineOption(const std::string& key,
                                                const std::string& description) -> ProgramDescription& {
  return defineOption<T>(key, '\0', description);
}

template <StringStreamable T>
constexpr auto ProgramDescription::defineOption(const std::string& key, char short_key,
                                                const std::string& description) -> ProgramDescription& {
  checkOptionAlreadyExists(key, short_key);

  options_.emplace_back(key, short_key, description, utils::getTypeName<T>(), "", true, false);
  return *this;
}

template <StringStreamable T>
constexpr auto ProgramDescription::defineOption(const std::string& key, const std::string& description,
                                                const T& default_value) -> ProgramDescription& {
  return defineOption<T>(key, '\0', description, default_value);
}

template <StringStreamable T>
constexpr auto ProgramDescription::defineOption(const std::string& key, char short_key,
                                                const std::string& description,
                                                const T& default_value) -> ProgramDescription& {
  checkOptionAlreadyExists(key, short_key);

  options_.emplace_back(key, short_key, description, utils::getTypeName<T>(),
                        std::format("{}", default_value), false, false);
  return *this;
}

constexpr auto ProgramDescription::defineFlag(const std::string& key, char short_key,
                                              const std::string& description) -> ProgramDescription& {
  checkOptionAlreadyExists(key, short_key);

  options_.emplace_back(key, short_key, description, utils::getTypeName<bool>(), "false", false, false);
  return *this;
}

constexpr auto ProgramDescription::defineFlag(const std::string& key,
                                              const std::string& description) -> ProgramDescription& {
  return defineFlag(key, '\0', description);
}

template <StringStreamable T>
inline auto ProgramOptions::getOption(const std::string& option) const -> T {
  const auto it = std::find_if(options_.begin(), options_.end(),
                               [&option](const auto& opt) { return option == opt.key; });
  throwExceptionIf<InvalidParameterException>(it == options_.end(),
                                              std::format("Undefined option '{}'", option));

  const auto my_type = utils::getTypeName<T>();
  throwExceptionIf<TypeMismatchException>(
      it->value_type != my_type,
      std::format("Tried to parse option '{}' as type {} but it's specified as type {}", option, my_type,
                  it->value_type));

  if constexpr (std::is_same_v<T, std::string>) {
    // note: since std::istringstream extracts only up to whitespace, this special case is
    // neccessary for parsing strings containing multiple words
    return it->value;
  }

  T value;
  std::istringstream stream(it->value);
  throwExceptionIf<TypeMismatchException>(
      not(stream >> std::boolalpha >> value),
      std::format("Unable to parse value '{}' as type {} for option '{}'", it->value, my_type, option));

  return value;
}
}  // namespace eolo::cli
