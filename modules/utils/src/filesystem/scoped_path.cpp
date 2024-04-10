#include "hephaestus/utils/filesystem/scoped_path.h"

#include <absl/random/random.h>
#include <absl/time/clock.h>
#include <fmt/core.h>

namespace heph::utils::filesystem {

ScopedPath::ScopedPath(std::filesystem::path path) : path_(std::move(path)), path_str_(path_.string()) {
}

ScopedPath::~ScopedPath() {
  std::error_code e;
  std::filesystem::remove_all(path_, e);
  if (e) {
    fmt::print(stderr, "Failed to remove test temporary directory {}, with error: {}\n", path_.string(),
               e.message());
  }
}

auto ScopedPath::createFile() -> ScopedPath {
  const auto global_temp_dir = std::filesystem::temp_directory_path();
  auto file = global_temp_dir / randomString();
  while (std::filesystem::exists(file)) {
    file = global_temp_dir / randomString();
  }

  return ScopedPath{ file };
}

auto ScopedPath::createDir() -> ScopedPath {
  const auto global_temp_dir = std::filesystem::temp_directory_path();
  auto temp_dir = global_temp_dir / randomString();
  while (std::filesystem::exists(temp_dir)) {
    temp_dir = global_temp_dir / randomString();
  }

  std::filesystem::create_directory(temp_dir);
  return ScopedPath{ std::move(temp_dir) };
}

ScopedPath::operator std::filesystem::path() const {
  return path_;
}

ScopedPath::operator std::string() const {
  return path_;
}

ScopedPath::operator std::string_view() const {
  return { path_str_ };
}

auto ScopedPath::randomString() -> std::string {
  absl::BitGen bitgen;
  return fmt::format("{}_{}", absl::ToUnixMillis(absl::Now()),
                     bitgen() % 1000);  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
}

}  // namespace heph::utils::filesystem
