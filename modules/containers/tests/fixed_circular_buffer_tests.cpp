//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <optional>

#include <gtest/gtest.h>

#include "hephaestus/containers/fixed_circular_buffer.h"

namespace heph::containers::tests {
// NOLINTBEGIN(bugprone-unchecked-optional-access)

struct NonCopyable {
  NonCopyable() = default;
  ~NonCopyable() = default;
  explicit NonCopyable(int dummy) : value(dummy) {
  }
  NonCopyable(const NonCopyable& other) = delete;
  auto operator=(const NonCopyable& other) -> NonCopyable& = delete;
  NonCopyable(NonCopyable&& other) = default;
  auto operator=(NonCopyable&& other) -> NonCopyable& = default;
  int value;
};
TEST(FixedCircularQueue, NonCopyable) {
  static constexpr int TEST_VALUE = 4711;
  static constexpr std::size_t QUEUE_SIZE = 16;
  FixedCircularBuffer<NonCopyable, QUEUE_SIZE> queue;
  EXPECT_TRUE(queue.tryPush(NonCopyable{ TEST_VALUE }));
  auto res = queue.tryPop();
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(res->value, TEST_VALUE);
}

TEST(FixedCircularQueue, Push) {
  {
    static constexpr std::size_t QUEUE_SIZE = 1;

    FixedCircularBuffer<int, QUEUE_SIZE> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.tryPush(0));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.tryPush(1));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 1);

    queue.forcePush(2);

    std::optional<int> res;

    res = queue.tryPop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 2);

    res = queue.tryPop();
    EXPECT_FALSE(res.has_value());
  }
  {
    static constexpr std::size_t QUEUE_SIZE = 2;

    FixedCircularBuffer<int, QUEUE_SIZE> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.tryPush(0));
    EXPECT_FALSE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 1);
    EXPECT_TRUE(queue.tryPush(1));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.tryPush(2));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 2);

    queue.forcePush(3);

    std::optional<int> res;

    res = queue.tryPop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 1);
    EXPECT_EQ(queue.size(), 1);

    EXPECT_TRUE(queue.tryPush(4));
    EXPECT_EQ(queue.size(), 2);
    res = queue.tryPop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 3);
    EXPECT_EQ(queue.size(), 1);

    res = queue.tryPop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 4);

    res = queue.tryPop();
    EXPECT_FALSE(res.has_value());
  }
}
// NOLINTEND(bugprone-unchecked-optional-access)
}  // namespace heph::containers::tests
