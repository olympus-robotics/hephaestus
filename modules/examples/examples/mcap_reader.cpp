//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#define MCAP_IMPLEMENTATION
#define MCAP_COMPRESSION_NO_ZSTD
#define MCAP_COMPRESSION_NO_LZ4
#include <filesystem>

#include <fmt/core.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <mcap/reader.hpp>

auto main(int argc, const char* argv[]) -> int {
  if (argc != 2) {
    fmt::println(stderr, "Usage: {} <input.mcap>",
                 argv[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::exit(1);
  }
  std::filesystem::path input{ argv[1] };  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

  mcap::McapReader reader;
  const auto res = reader.open(input.string());
  if (!res.ok()) {
    fmt::println(stderr, "Failed to open: {} for writing", input.c_str());
    std::exit(1);
  }
}
