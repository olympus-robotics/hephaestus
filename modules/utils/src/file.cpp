#include "hephaestus/utils/file.h"

#include <fstream>

#include <absl/random/random.h>
#include <absl/time/clock.h>
#include <fmt/core.h>

#include "hephaestus/utils/exception.h"

namespace heph::utils {
auto readFile(const std::filesystem::path& path) -> std::string {
  std::ifstream infile{ path };
  throwExceptionIf<InvalidDataException>(
      !infile, fmt::format("Could not open file {} to read content", path.string()));

  std::string text;
  text.reserve(std::filesystem::file_size(path));
  text.assign(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>());

  return text;
}

auto readBinaryFile(const std::filesystem::path& path) -> std::vector<std::byte> {
  std::ifstream infile{ path, std::ios::binary };
  throwExceptionIf<InvalidDataException>(
      !infile, fmt::format("Could not open file {} to read content", path.string()));

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

void writeBufferToFile(const std::filesystem::path& path, std::span<std::byte> content) {
  std::ofstream outfile{ path, std::ios::out | std::ios::binary };
  throwExceptionIf<InvalidDataException>(!outfile,
                                         fmt::format("Could not open file {} to write", path.string()));

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  outfile.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
}

// -------------------------------------------------------------------------------------------------
// SelfDestructingPath implementation
// -------------------------------------------------------------------------------------------------

SelfDestructingPath::SelfDestructingPath(std::filesystem::path path)
  : path_(std::move(path)), path_str_(path_.string()) {
}

SelfDestructingPath::~SelfDestructingPath() {
  std::error_code e;
  std::filesystem::remove_all(path_, e);
  if (e) {
    fmt::print(stderr, "failed to remove test temporary directory {}, with error: {}\n", path_.string(),
               e.message());
  }
}

auto SelfDestructingPath::tempFile() -> SelfDestructingPath {
  const auto global_temp_dir = std::filesystem::temp_directory_path();
  return SelfDestructingPath{ global_temp_dir / randomString() };
}

auto SelfDestructingPath::tempDir() -> SelfDestructingPath {
  const auto global_temp_dir = std::filesystem::temp_directory_path();
  auto temp_dir = global_temp_dir / randomString();
  std::filesystem::create_directory(temp_dir);
  return SelfDestructingPath{ std::move(temp_dir) };
}

SelfDestructingPath::operator std::filesystem::path() const {
  return path_;
}

SelfDestructingPath::operator std::string() const {
  return path_;
}

SelfDestructingPath::operator std::string_view() const {
  return { path_str_ };
}

auto SelfDestructingPath::randomString() -> std::string {
  absl::BitGen bitgen;
  return fmt::format("{}_{}", absl::ToUnixMillis(absl::Now()),
                     bitgen() % 1000);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
}

}  // namespace heph::utils
