//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <exec/static_thread_pool.hpp>
#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/concurrency/any_sender.h"
#include "hephaestus/concurrency/when_all_range.h"

namespace heph::concurrency {
TEST(WhenAllRange, Empty) {
  std::vector<AnySender<void>> senders;

  stdexec::sync_wait(whenAllRange(std::move(senders)));
}
TEST(WhenAllRange, Basic) {
  std::vector<AnySender<void>> senders;

  std::size_t completed{ 0 };
  static constexpr std::size_t NUMBER_OF_SENDERS{ 100 };
  for (std::size_t i = 0; i != NUMBER_OF_SENDERS; ++i) {
    senders.emplace_back(stdexec::just() | stdexec::then([&completed]() { ++completed; }));
  }

  stdexec::sync_wait(whenAllRange(std::move(senders)));

  EXPECT_EQ(completed, NUMBER_OF_SENDERS);
}

TEST(WhenAllRange, Stop) {
  std::vector<AnySender<void>> senders;

  std::size_t completed{ 0 };
  static constexpr std::size_t NUMBER_OF_SENDERS{ 100 };
  for (std::size_t i = 0; i != NUMBER_OF_SENDERS; ++i) {
    if (i == NUMBER_OF_SENDERS / 2) {
      senders.emplace_back(stdexec::just_stopped() | stdexec::then([&completed]() { ++completed; }));
    } else {
      senders.emplace_back(stdexec::just() | stdexec::then([&completed]() { ++completed; }));
    }
  }

  auto res = stdexec::sync_wait(whenAllRange(std::move(senders)));
  EXPECT_FALSE(res.has_value());

  EXPECT_EQ(completed, NUMBER_OF_SENDERS - 1);
}

TEST(WhenAllRange, Concurrent) {
  std::vector<AnySender<void>> senders;
  exec::static_thread_pool pool{ 4 };

  std::size_t completed{ 0 };
  static constexpr std::size_t NUMBER_OF_SENDERS{ 100 };
  for (std::size_t i = 0; i != NUMBER_OF_SENDERS; ++i) {
    senders.emplace_back(stdexec::schedule(pool.get_scheduler()) |
                         stdexec::then([&completed]() { ++completed; }));
  }

  stdexec::sync_wait(whenAllRange(std::move(senders)));

  EXPECT_EQ(completed, NUMBER_OF_SENDERS);
}
}  // namespace heph::concurrency
