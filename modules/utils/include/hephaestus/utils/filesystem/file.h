#pragma once
#include <filesystem>
#include <span>

namespace heph::utils::filesystem {

[[nodiscard]] auto readFile(const std::filesystem::path& path) -> std::string;
[[nodiscard]] auto readBinaryFile(const std::filesystem::path& path) -> std::vector<std::byte>;

void writeStringToFile(const std::filesystem::path& path, std::string_view content);
void writeBufferToFile(const std::filesystem::path& path, std::span<const std::byte> content);

/// This class allows to create a file that is removed when the class goes out of scope (RAII).
/// This is very useful in tests to avoid having dangling files.
class ScopedPath {
public:
  explicit ScopedPath(std::filesystem::path path);
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
  [[nodiscard]] static auto randomString() -> std::string;

  std::filesystem::path path_;
  std::string path_str_;
};

}  // namespace heph::utils::filesystem
