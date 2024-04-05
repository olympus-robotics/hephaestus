//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/utils/utils.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::utils::tests {
TEST(BinaryFile, ReadWrite) {
  const auto path = SelfDestructingPath::tempFile();
  const auto content = std::vector<std::byte>{ std::byte{ 0x01 }, std::byte{ 0x02 }, std::byte{ 0x03 } };

  writeBufferToFile(path, content);
  const auto read_content = readBinaryFile(path);

  EXPECT_EQ(content, read_content);
}

TEST(File, ReadWrite) {
  const auto path = SelfDestructingPath::tempFile();
  const auto content = std::string{ "Hello, World!" };

  writeStringToFile(path, content);
  const auto read_content = readFile(path);

  EXPECT_EQ(content, read_content);
}
}  // namespace heph::utils::tests
