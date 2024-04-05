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
class SelfDestructingPath {
public:
  explicit SelfDestructingPath(std::filesystem::path path);
  ~SelfDestructingPath();
  SelfDestructingPath(const SelfDestructingPath&) = delete;
  SelfDestructingPath(SelfDestructingPath&&) = default;
  auto operator=(const SelfDestructingPath&) -> SelfDestructingPath& = delete;
  auto operator=(SelfDestructingPath&&) -> SelfDestructingPath& = default;

  [[nodiscard]] static auto createFile() -> SelfDestructingPath;

  [[nodiscard]] static auto createDir() -> SelfDestructingPath;

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
