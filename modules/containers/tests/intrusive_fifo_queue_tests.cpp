//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/containers/intrusive_fifo_queue.h"

namespace heph::containers {

struct Dummy {
  Dummy* next{ nullptr };
  Dummy* prev{ nullptr };
};

TEST(IntrusiveFifoQueue, Empty) {
  IntrusiveFifoQueue<Dummy> queue;
  EXPECT_EQ(queue.size(), 0);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.dequeue(), nullptr);
}

TEST(IntrusiveFifoQueue, EnqueueDequeue) {
  IntrusiveFifoQueue<Dummy> queue;

  Dummy dummy1;
  Dummy dummy2;
  Dummy dummy3;
  Dummy dummy4;

  queue.enqueue(&dummy1);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);

  queue.enqueue(&dummy1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  queue.enqueue(&dummy2);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 2);
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy2);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);

  queue.enqueue(&dummy1);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  queue.enqueue(&dummy2);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.empty());
  queue.enqueue(&dummy3);
  EXPECT_EQ(queue.size(), 3);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.empty());
  queue.enqueue(&dummy4);
  EXPECT_EQ(queue.size(), 3);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy2);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy3);
  EXPECT_EQ(queue.size(), 1);
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.dequeue(), &dummy4);
  EXPECT_EQ(queue.size(), 0);
  EXPECT_TRUE(queue.empty());
}
TEST(IntrusiveFifoQueue, Erase) {
  IntrusiveFifoQueue<Dummy> queue;

  Dummy dummy1;
  Dummy dummy2;
  Dummy dummy3;
  Dummy dummy4;

  EXPECT_FALSE(queue.erase(&dummy1));

  queue.enqueue(&dummy1);
  EXPECT_TRUE(queue.erase(&dummy1));
  EXPECT_FALSE(queue.erase(&dummy1));
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(dummy1.next, nullptr);

  queue.enqueue(&dummy1);
  queue.enqueue(&dummy2);
  EXPECT_TRUE(queue.erase(&dummy1));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_EQ(dummy1.next, nullptr);
  EXPECT_EQ(queue.dequeue(), &dummy2);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(dummy2.next, nullptr);

  queue.enqueue(&dummy1);
  queue.enqueue(&dummy2);
  EXPECT_TRUE(queue.erase(&dummy2));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 1);
  EXPECT_EQ(dummy2.next, nullptr);
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(dummy1.next, nullptr);

  queue.enqueue(&dummy1);
  queue.enqueue(&dummy2);
  queue.enqueue(&dummy3);
  EXPECT_TRUE(queue.erase(&dummy2));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 2);
  EXPECT_EQ(dummy2.next, nullptr);
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_EQ(queue.dequeue(), &dummy3);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(dummy1.next, nullptr);
  EXPECT_EQ(dummy3.next, nullptr);

  queue.enqueue(&dummy1);
  queue.enqueue(&dummy2);
  queue.enqueue(&dummy3);
  queue.enqueue(&dummy4);
  EXPECT_TRUE(queue.erase(&dummy4));
  EXPECT_FALSE(queue.empty());
  EXPECT_EQ(queue.size(), 3);
  EXPECT_EQ(dummy4.next, nullptr);
  EXPECT_EQ(queue.dequeue(), &dummy1);
  EXPECT_EQ(queue.dequeue(), &dummy2);
  EXPECT_EQ(queue.dequeue(), &dummy3);
  EXPECT_TRUE(queue.empty());
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(dummy1.next, nullptr);
  EXPECT_EQ(dummy2.next, nullptr);
  EXPECT_EQ(dummy3.next, nullptr);
}

class DummyPrivate {
private:
  friend struct IntrusiveFifoQueueAccess;
  [[maybe_unused]] DummyPrivate* next_{ nullptr };
  [[maybe_unused]] DummyPrivate* prev_{ nullptr };
};
TEST(IntrusiveFifoQueue, Access) {
  IntrusiveFifoQueue<DummyPrivate> queue;

  DummyPrivate dummy;
  queue.enqueue(&dummy);
  EXPECT_EQ(queue.dequeue(), &dummy);
}

class DummyPrivateNoAccess {
private:
  [[maybe_unused]] DummyPrivate* next_{ nullptr };
};

TEST(IntrusiveFifoQueue, Concepts) {
  EXPECT_TRUE(heph::containers::IntrusiveFifoQueueElement<Dummy>);
  EXPECT_TRUE(heph::containers::IntrusiveFifoQueueElement<DummyPrivate>);
  EXPECT_FALSE(heph::containers::IntrusiveFifoQueueElement<DummyPrivateNoAccess>);
}
}  // namespace heph::containers
