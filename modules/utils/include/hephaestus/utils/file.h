#pragma once
#include <filesystem>
#include <span>

namespace heph::utils {

[[nodiscard]] auto readFile(const std::filesystem::path& path) -> std::string;
[[nodiscard]] auto readBinaryFile(const std::filesystem::path& path) -> std::vector<std::byte>;

void writeStringToFile(const std::filesystem::path& path, std::string_view content);
void writeBufferToFile(const std::filesystem::path& path, std::span<const std::byte> content);

/// This class allows to create a file that is removed when the class goes out of scope (RAII).
/// This is very useful in tests to avoid having dangling files.
class ScopedFilesystemPath {
public:
  explicit ScopedFilesystemPath(std::filesystem::path path);
  ~ScopedFilesystemPath();
  ScopedFilesystemPath(const ScopedFilesystemPath&) = delete;
  ScopedFilesystemPath(ScopedFilesystemPath&&) = default;
  auto operator=(const ScopedFilesystemPath&) -> ScopedFilesystemPath& = delete;
  auto operator=(ScopedFilesystemPath&&) -> ScopedFilesystemPath& = default;

  [[nodiscard]] static auto createFile() -> ScopedFilesystemPath;

  [[nodiscard]] static auto createDir() -> ScopedFilesystemPath;

  operator std::filesystem::path() const;  // NOLINT(google-explicit-conversion,
                                           // google-explicit-constructor)

  operator std::string() const;  // NOLINT(google-explicit-conversion,
                                 // google-explicit-constructor)

  operator std::string_view() const;  // NOLINT(google-explicit-conversion,
                                      // google-explicit-constructor)

private:
  [[nodiscard]] static auto randomString() -> std::string;

  std::filesystem::path path_;
  std::string path_str_;
};

}  // namespace heph::utils
