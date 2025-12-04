//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace heph::utils::filesystem {

/// This class allows to create a file that is removed when the class goes out of scope (RAII).
/// This is very useful in tests to avoid having dangling files.
class ScopedPath {
public:
  ~ScopedPath();
  ScopedPath(const ScopedPath&) = delete;
  ScopedPath(ScopedPath&&) = default;
  auto operator=(const ScopedPath&) -> ScopedPath& = delete;
  auto operator=(ScopedPath&&) -> ScopedPath& = default;

  [[nodiscard]] static auto createFile() -> ScopedPath;

  [[nodiscard]] static auto createDir() -> ScopedPath;

  operator std::filesystem::path() const;  // NOLINT(google-explicit-conversion,
                                           // google-explicit-constructor)

  operator std::string() const;  // NOLINT(google-explicit-conversion,
                                 // google-explicit-constructor)

  operator std::string_view() const;  // NOLINT(google-explicit-conversion,
                                      // google-explicit-constructor)

private:
  explicit ScopedPath(std::filesystem::path path);

  [[nodiscard]] static auto randomString() -> std::string;

  std::filesystem::path path_;
  std::string path_str_;
};

}  // namespace heph::utils::filesystem
