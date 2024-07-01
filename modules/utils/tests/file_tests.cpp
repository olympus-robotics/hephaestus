//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/utils/filesystem/file.h"
#include "hephaestus/utils/filesystem/scoped_path.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::filesystem::tests {
TEST(Filesystem, ReadWriteBinaryFile) {
  const auto path = ScopedPath::createFile();
  const auto content = std::vector<std::byte>{ std::byte{ 0x01 }, std::byte{ 0x02 }, std::byte{ 0x03 } };

  writeBufferToFile(path, { content.data(), content.size() });
  const auto read_content = readBinaryFile(path);

  EXPECT_EQ(content, read_content);
}

TEST(Filesystem, ReadWriteTextFile) {
  const auto path = ScopedPath::createFile();
  const auto content = std::string{ "Hello, World!" };

  writeStringToFile(path, content);
  const auto read_content = readFile(path);

  EXPECT_EQ(content, read_content);
}
}  // namespace heph::utils::filesystem::tests
