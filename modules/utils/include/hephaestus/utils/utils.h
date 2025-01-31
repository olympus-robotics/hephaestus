//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <cxxabi.h>

namespace heph::utils {

/// Return user-readable name for specified type
template <typename T>
auto getTypeName() -> std::string {
  // From https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
  const auto* const mangled_name = typeid(T).name();
  int status{ 0 };
  const std::unique_ptr<char, void (*)(void*)> res{
    abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status), std::free
  };
  return (status == 0) ? res.get() : mangled_name;
}

[[nodiscard]] auto getHostName() -> std::string;

[[nodiscard]] auto getBinaryPath() -> std::optional<std::filesystem::path>;

}  // namespace heph::utils
