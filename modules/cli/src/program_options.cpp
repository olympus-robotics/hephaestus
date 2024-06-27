//=================================================================================================
// Copyright(C) 2018 GRAPE Contributors
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include "hephaestus/cli/program_options.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <fmt/core.h>

#include "hephaestus/utils/exception.h"
#include "hephaestus/utils/utils.h"

namespace heph::cli {
namespace {
auto getKeyStringForHelper(const ProgramOptions::Option& option) -> std::string {
  if (option.short_key == '\0') {
    return fmt::format("--{}   ", option.key);
  }

  return fmt::format("--{} -{}", option.key, option.short_key);
}
}  // namespace

ProgramOptions::ProgramOptions(std::vector<Option>&& options) : options_(std::move(options)) {
}

auto ProgramOptions::hasOption(const std::string& option) const -> bool {
  return (options_.end() != std::find_if(options_.begin(), options_.end(),
                                         [&option](const auto& opt) { return option == opt.key; }));
}

ProgramDescription::ProgramDescription(std::string brief) : brief_(std::move(brief)) {
  options_.emplace_back(HELP_KEY, HELP_SHORT_KEY, "", utils::getTypeName<std::string>(), "", false, false);
}

void ProgramDescription::checkOptionAlreadyExists(const std::string& key, char k) const {
  const auto it =
      std::find_if(options_.begin(), options_.end(), [&key](const auto& opt) { return key == opt.key; });
  throwExceptionIf<InvalidOperationException>(it != options_.end(),
                                              fmt::format("Attempted redefinition of option '{}'", key));

  if (k == '\0') {
    return;
  }

  const auto short_it =
      std::find_if(options_.begin(), options_.end(), [k](const auto& opt) { return k == opt.short_key; });
  throwExceptionIf<InvalidOperationException>(
      short_it != options_.end(),
      fmt::format("Attempted redefinition of short key '{}' for option '{}'", k, key));
}

auto ProgramDescription::defineFlag(const std::string& key, char short_key,
                                    const std::string& description) -> ProgramDescription& {
  checkOptionAlreadyExists(key, short_key);

  options_.emplace_back(key, short_key, description, utils::getTypeName<bool>(), "false", false, false);
  return *this;
}

auto ProgramDescription::defineFlag(const std::string& key,
                                    const std::string& description) -> ProgramDescription& {
  return defineFlag(key, '\0', description);
}

auto ProgramDescription::parse(int argc, const char** argv) && -> ProgramOptions {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const auto args = std::vector<std::string>(argv + 1, argv + argc);
  return std::move(*this).parse(args);
}

auto ProgramDescription::parse(const std::vector<std::string>& args) && -> ProgramOptions {
  auto& help_option = options_.front();  // Help option is always the first.
  help_option.value = getHelpMessage();

  for (auto arg_it = args.begin(); arg_it != args.end(); ++arg_it) {
    auto& option = getOptionFromArg(*arg_it);
    if (option.key == HELP_KEY) {
      fmt::println(stderr, "{}", help_option.value);
      exit(0);
    }

    // If option is a flag skip next.
    if (option.value_type == utils::getTypeName<bool>()) {
      option.value = "true";
      continue;
    }

    ++arg_it;
    throwExceptionIf<InvalidParameterException>(
        arg_it == args.end(), fmt::format("After option --{} there is supposed to be a value", option.key));
    throwExceptionIf<InvalidParameterException>(
        arg_it->starts_with('-'),
        fmt::format("Option --{} is supposed to be followed by a value, not another option {}", option.key,
                    *arg_it));

    option.value = *arg_it;
    option.is_specified = true;
  }

  // check all required arguments are specified
  for (const auto& entry : options_) {
    throwExceptionIf<InvalidConfigurationException>(
        entry.is_required and not entry.is_specified,
        fmt::format("Required option '{}' not specified", entry.key));
  }

  return ProgramOptions(std::move(options_));
}

auto ProgramDescription::getOptionFromArg(const std::string& arg) -> ProgramOptions::Option& {
  if (arg.starts_with("--")) {
    const auto key = arg.substr(2);
    const auto it =
        std::find_if(options_.begin(), options_.end(), [&key](const auto& opt) { return key == opt.key; });

    throwExceptionIf<InvalidParameterException>(it == options_.end(),
                                                fmt::format("Undefined option '{}'", key));
    return *it;
  }

  if (arg.starts_with("-")) {
    throwExceptionIf<InvalidParameterException>(arg.size() != 2,
                                                fmt::format("Undefined option '{}'", arg.substr(1)));
    const auto short_key = arg[1];
    const auto it = std::find_if(options_.begin(), options_.end(),
                                 [short_key](const auto& opt) { return short_key == opt.short_key; });
    return *it;
  }

  throwException<InvalidParameterException>(
      fmt::format("Arg {} is not a valid option, it must start with either '--' or '-'", arg));

  // NOTE: this will never happen as we will throw an exception before getting here, but compiler
  // wants this.
  return options_.front();
}

auto ProgramDescription::getHelpMessage() const -> std::string {
  std::stringstream help_stream;
  help_stream << brief_ << "\nOptions:\n";
  for (const auto& entry : options_) {
    if (entry.key == HELP_KEY) {
      continue;
    }
    if (entry.is_required) {
      help_stream << fmt::format("{} [required]: {}. [type: {}]\n", getKeyStringForHelper(entry),
                                 entry.description, entry.value_type);
    } else {
      help_stream << fmt::format("{} [optional]: {}; (default: {}) [type: {}]\n",
                                 getKeyStringForHelper(entry), entry.description, entry.value,
                                 entry.value_type);
    }
  }

  help_stream << fmt::format("--{} -{} [optional]: {}", HELP_KEY, HELP_SHORT_KEY, "This text!");

  return help_stream.str();
}

}  // namespace heph::cli
