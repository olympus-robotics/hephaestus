#include "hephaestus/utils/filesystem/file.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>

#include "hephaestus/utils/exception.h"

namespace heph::utils::filesystem {
auto readFile(const std::filesystem::path& path) -> std::string {
  std::ifstream infile{ path };
  throwExceptionIf<InvalidDataException>(!infile,
                                         fmt::format("Could not open file {} to read", path.string()));

  std::string text;
  text.reserve(std::filesystem::file_size(path));
  text.assign(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

  return text;
}

auto readBinaryFile(const std::filesystem::path& path) -> std::vector<std::byte> {
  std::ifstream infile{ path, std::ios::binary };
  throwExceptionIf<InvalidDataException>(!infile,
                                         fmt::format("Could not open file {} to read", path.string()));

  const auto file_size = std::filesystem::file_size(path);
  std::vector<std::byte> buffer(file_size);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  infile.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size));

  return buffer;
}

void writeStringToFile(const std::filesystem::path& path, std::string_view content) {
  std::ofstream outfile{ path, std::ios::out };
  throwExceptionIf<InvalidDataException>(!outfile,
                                         fmt::format("Could not open file {} to write", path.string()));

  outfile.write(content.data(), static_cast<std::streamsize>(content.size()));
}

void writeBufferToFile(const std::filesystem::path& path, std::span<const std::byte> content) {
  std::ofstream outfile{ path, std::ios::out | std::ios::binary };
  throwExceptionIf<InvalidDataException>(!outfile,
                                         fmt::format("Could not open file {} to write", path.string()));

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  outfile.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
}

}  // namespace heph::utils::filesystem
