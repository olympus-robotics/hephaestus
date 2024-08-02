//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstddef>
#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/utils/filesystem/file.h"
#include "hephaestus/utils/filesystem/scoped_path.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::filesystem::tests {
TEST(Filesystem, WriteFileFail) {
  const std::filesystem::path output_file =
      "/tmp/this_folder_should_not_exist/this_file_should_not_exist.txt";
  ASSERT_FALSE(std::filesystem::exists(output_file));

  auto res = writeStringToFile(output_file, "Hello, World!");
  EXPECT_FALSE(res);
  res = writeBufferToFile(output_file, {});
  EXPECT_FALSE(res);
}

TEST(Filesystem, ReadFileFail) {
  const std::filesystem::path output_file =
      "/tmp/this_folder_should_not_exist/this_file_should_not_exist.txt";
  ASSERT_FALSE(std::filesystem::exists(output_file));

  const auto content = readFile(output_file);
  EXPECT_FALSE(content);
  const auto buffer = readBinaryFile(output_file);
  EXPECT_FALSE(buffer);
}

TEST(Filesystem, ReadWriteBinaryFile) {
  const auto path = ScopedPath::createFile();
  const auto content = std::vector<std::byte>{ std::byte{ 0x01 }, std::byte{ 0x02 }, std::byte{ 0x03 } };

  auto res = writeBufferToFile(path, { content.data(), content.size() });
  EXPECT_TRUE(res);
  const auto read_content = readBinaryFile(path);

  EXPECT_EQ(content, read_content);
}

TEST(Filesystem, ReadWriteTextFile) {
  const auto path = ScopedPath::createFile();
  const auto content = std::string{ "Hello, World!" };

  auto res = writeStringToFile(path, content);
  EXPECT_TRUE(res);
  const auto read_content = readFile(path);

  EXPECT_EQ(content, read_content);
}
}  // namespace heph::utils::filesystem::tests
