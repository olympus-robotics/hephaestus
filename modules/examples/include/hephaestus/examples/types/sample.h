//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <cstddef>

#include <fmt/format.h>
namespace heph::examples::types {

struct SampleRequest {
  std::size_t initial_value{ 0 };
  std::size_t iterations_count{ 0 };
};

struct SampleReply {
  std::size_t value{ 0 };
  std::size_t counter{ 0 };
};

// NOLINTNEXTLINE(readability-identifier-naming)
static inline auto format_as(const SampleRequest& sample) -> std::string {
  return fmt::format("initial value: {} | iterations: {}", sample.initial_value, sample.iterations_count);
}

// NOLINTNEXTLINE(readability-identifier-naming)
static inline auto format_as(const SampleReply& sample) -> std::string {
  return fmt::format("value: {} | counter: {}", sample.value, sample.counter);
}

}  // namespace heph::examples::types
