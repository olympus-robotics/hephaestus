//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/cli/program_options.h"

#include "eolo/base/exception.h"

namespace eolo::cli {
namespace {
auto getKeyStringForHelper(const ProgramOptions::Option& option) -> std::string {
  if (option.short_key == '\0') {
    return std::format("--{}   ", option.key);
  }

  return std::format("--{} -{}", option.key, option.short_key);
}
}  // namespace

ProgramOptions::ProgramOptions(std::vector<Option>&& options) : options_(std::move(options)) {
}

auto ProgramOptions::hasOption(const std::string& option) const -> bool {
  return (options_.end() != std::find_if(options_.begin(), options_.end(),
                                         [&option](const auto& opt) { return option == opt.key; }));
}

auto ProgramDescription::getOptionFromArg(const std::string& arg) -> ProgramOptions::Option& {
  if (arg.starts_with("--")) {
    const auto key = arg.substr(2);
    const auto it = std::find_if(options_.begin(), options_.end(),
                                 [&key](const auto& opt) { return key == opt.key; });

    throwExceptionIf<InvalidParameterException>(it == options_.end(),
                                                std::format("Undefined option '{}'", key));
    return *it;
  }

  if (arg.starts_with("-")) {
    throwExceptionIf<InvalidParameterException>(
        arg.size() != 2, std::format("Undefined option '{}'", arg.substr(1)));
    const auto short_key = arg[1];
    const auto it = std::find_if(options_.begin(), options_.end(), [short_key](const auto& opt) {
      return short_key == opt.short_key;
    });
    return *it;
  }

  throwException<InvalidParameterException>(
      std::format("Arg {} is not a valid option, it must start with either '--' or '-'", arg));

  // NOTE: this will never happen as we will throw an exception before getting here, but compiler
  // wants this.
  return options_.front();
}

auto ProgramDescription::parse(int argc, const char** argv) && -> ProgramOptions {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const auto args = std::vector<std::string>(argv, argv + argc);
  return std::move(*this).parse(args);
}

auto ProgramDescription::parse(const std::vector<std::string>& args) && -> ProgramOptions {
  for (auto arg_it = args.begin(); arg_it != args.end(); ++arg_it) {
    auto& option = getOptionFromArg(*arg_it);
    if (option.key == HELP_KEY) {
      std::println(stderr, "{}", getHelpMessage());
      exit(0);
    }

    // If option is a flag skip next.
    ++arg_it;
    throwExceptionIf<InvalidParameterException>(
        arg_it == args.end(),
        std::format("After option --{} there is supposed to be a value", option.key));
    throwExceptionIf<InvalidParameterException>(
        arg_it->starts_with('-'),
        std::format("Option --{} is supposed to be followed by a value, not another option {}",
                    option.key, *arg_it));

    option.value = *arg_it;
    option.is_specified = true;
  }

  // check all required arguments are specified
  for (const auto& entry : options_) {
    throwExceptionIf<InvalidConfigurationException>(
        entry.is_required and not entry.is_specified,
        std::format("Required option '{}' not specified", entry.key));
  }

  return ProgramOptions(std::move(options_));
}

auto ProgramDescription::getHelpMessage() const -> std::string {
  std::stringstream help_stream;
  help_stream << "\nOptions:\n";
  for (const auto& entry : options_) {
    if (entry.key == HELP_KEY) {
      continue;
    }
    if (entry.is_required) {
      help_stream << std::format("{} [required]: {}. [type: {}]\n", getKeyStringForHelper(entry),
                                 entry.description, entry.value_type);
    } else {
      help_stream << std::format("{} [optional]: {}; (default: {}) [type: {}]\n",
                                 getKeyStringForHelper(entry), entry.description, entry.value,
                                 entry.value_type);
    }
  }

  help_stream << std::format("--{} -h [optional]: {}", HELP_KEY, HELP_SHORT_KEY, "This text!");

  return help_stream.str();
}

}  // namespace eolo::cli
