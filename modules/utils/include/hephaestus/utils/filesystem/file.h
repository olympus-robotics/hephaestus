#pragma once
#include <filesystem>
#include <span>

namespace heph::utils::filesystem {

[[nodiscard]] auto readFile(const std::filesystem::path& path) -> std::string;
[[nodiscard]] auto readBinaryFile(const std::filesystem::path& path) -> std::vector<std::byte>;

void writeStringToFile(const std::filesystem::path& path, std::string_view content);
void writeBufferToFile(const std::filesystem::path& path, std::span<const std::byte> content);

}  // namespace heph::utils::filesystem
