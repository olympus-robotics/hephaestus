#include "hephaestus/utils/filesystem/file.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifdef __linux__
#include <linux/limits.h>
#endif

#include <fmt/format.h>
#include <unistd.h>

namespace heph::utils::filesystem {
auto readFile(const std::filesystem::path& path) -> std::optional<std::string> {
  std::ifstream infile{ path };
  if (!infile) {
    return {};
  }

  std::string text;
  text.reserve(std::filesystem::file_size(path));
  text.assign(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

  return text;
}

auto readBinaryFile(const std::filesystem::path& path) -> std::optional<std::vector<std::byte>> {
  std::ifstream infile{ path, std::ios::binary };
  if (!infile) {
    return {};
  }

  const auto file_size = std::filesystem::file_size(path);
  std::vector<std::byte> buffer(file_size);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  infile.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));

  return buffer;
}

auto writeStringToFile(const std::filesystem::path& path, std::string_view content) -> bool {
  std::ofstream outfile{ path, std::ios::out };
  if (!outfile) {
    return false;
  }

  outfile.write(content.data(), static_cast<std::streamsize>(content.size()));
  return true;
}

auto writeBufferToFile(const std::filesystem::path& path, std::span<const std::byte> content) -> bool {
  std::ofstream outfile{ path, std::ios::out | std::ios::binary };
  if (!outfile) {
    return false;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  outfile.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
  return true;
}

auto searchFilenameInPaths(const std::string& filename, const std::vector<std::filesystem::path>& paths)
    -> std::optional<std::filesystem::path> {
  if (std::filesystem::exists(filename)) {
    return filename;
  }

  for (const auto& path : paths) {
    auto file = path / filename;
    if (std::filesystem::exists(file)) {
      return file;
    }
  }

  return {};
}

auto getThisExecutableFullPath() -> std::filesystem::path {
/// \note This implementation only works for Linux
#ifdef __linux__
  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  char path[PATH_MAX + 1] = { '\0' };  // NOLINT(cppcoreguidelines-avoid-c-arrays)
  const auto path_length = readlink("/proc/self/exe", path, PATH_MAX);
  return ((path_length > 0) ? std::filesystem::path{ path } : (""));
#elif
  return {};
#endif
  // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
}

}  // namespace heph::utils::filesystem
