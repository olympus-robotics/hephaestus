//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <optional>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/containers/fixed_circular_buffer.h"

namespace heph::containers::tests {
// NOLINTBEGIN(bugprone-unchecked-optional-access)
TEST(FixedCircularQueue, Push) {
  {
    static constexpr std::size_t QUEUE_SIZE = 1;

    FixedCircularBuffer<int, QUEUE_SIZE> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.push(0));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.push(1));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 1);

    queue.pushForce(2);

    std::optional<int> res;

    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 2);

    res = queue.pop();
    EXPECT_FALSE(res.has_value());
  }
  {
    static constexpr std::size_t QUEUE_SIZE = 2;

    FixedCircularBuffer<int, QUEUE_SIZE> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.push(0));
    EXPECT_FALSE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 1);
    EXPECT_TRUE(queue.push(1));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.push(2));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 2);

    queue.pushForce(3);

    std::optional<int> res;

    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 1);
    EXPECT_EQ(queue.size(), 1);

    EXPECT_TRUE(queue.push(4));
    EXPECT_EQ(queue.size(), 2);
    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 3);
    EXPECT_EQ(queue.size(), 1);

    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 4);

    res = queue.pop();
    EXPECT_FALSE(res.has_value());
  }
}
TEST(FixedCircularQueue, PushSpsc) {
  {
    static constexpr std::size_t QUEUE_SIZE = 1;

    FixedCircularBuffer<int, QUEUE_SIZE, FixedCircularBufferMode::SPSC> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.push(0));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.push(1));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 1);

    std::optional<int> res;

    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 0);

    EXPECT_TRUE(queue.push(1));
    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 1);

    res = queue.pop();
    EXPECT_FALSE(res.has_value());
  }
  {
    static constexpr std::size_t QUEUE_SIZE = 2;

    FixedCircularBuffer<int, QUEUE_SIZE, FixedCircularBufferMode::SPSC> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.push(0));
    EXPECT_FALSE(queue.empty());
    EXPECT_FALSE(queue.full());
    EXPECT_EQ(queue.size(), 1);
    EXPECT_TRUE(queue.push(1));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 2);
    EXPECT_FALSE(queue.push(2));
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(queue.full());
    EXPECT_EQ(queue.size(), 2);

    std::optional<int> res;

    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 0);
    EXPECT_EQ(queue.size(), 1);

    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 1);
    EXPECT_EQ(queue.size(), 0);

    EXPECT_TRUE(queue.push(2));
    EXPECT_EQ(queue.size(), 1);
    res = queue.pop();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(*res, 2);
    EXPECT_EQ(queue.size(), 0);

    res = queue.pop();
    EXPECT_FALSE(res.has_value());
  }
}
TEST(FixedCircularQueue, PushSpscConcurrent) {
  static constexpr std::size_t NUMBER_OF_ITERATIONS = 100000;
  static constexpr std::size_t QUEUE_SIZE = 2;

  FixedCircularBuffer<std::size_t, QUEUE_SIZE, FixedCircularBufferMode::SPSC> queue;

  std::thread producer{ [&]() {
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      while (!queue.push(i)) {
        ;  // the other thread should pop eventually
      }
    }
  } };
  std::thread consumer{ [&]() {
    for (std::size_t i = 0; i != NUMBER_OF_ITERATIONS; ++i) {
      while (true) {
        auto res = queue.pop();
        if (res.has_value()) {
          EXPECT_EQ(res.value(), i);
          break;
        }
      }
    }
  } };

  producer.join();
  consumer.join();
}
// NOLINTEND(bugprone-unchecked-optional-access)
}  // namespace heph::containers::tests
