//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace heph::utils::filesystem {

/// Read the whole content of the input file into a string.
/// \return std::nullopt if the file doesn't exists.
[[nodiscard]] auto readFile(const std::filesystem::path& path) -> std::optional<std::string>;
/// Read the whole content of the input binary file into a binary buffer.
/// \return std::nullopt if the file doesn't exists.
[[nodiscard]] auto readBinaryFile(const std::filesystem::path& path) -> std::optional<std::vector<std::byte>>;

[[nodiscard]] auto writeStringToFile(const std::filesystem::path& path, std::string_view content) -> bool;
[[nodiscard]] auto writeBufferToFile(const std::filesystem::path& path, std::span<const std::byte> content)
    -> bool;

[[nodiscard]] auto searchFilenameInPaths(const std::string& filename,
                                         const std::vector<std::filesystem::path>& paths)
    -> std::optional<std::filesystem::path>;

/// Return the full path of the executable calling this function.
[[nodiscard]] auto getThisExecutableFullPath() -> std::filesystem::path;

}  // namespace heph::utils::filesystem
