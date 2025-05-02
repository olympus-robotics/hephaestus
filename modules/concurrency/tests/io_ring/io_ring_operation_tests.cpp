//=================================================================================================
// Copyright (C) 2023-2025 HEPHAESTUS Contributors
//=================================================================================================

#include <cstddef>
#include <cstring>

#include <gtest/gtest.h>
#include <liburing.h>  // NOLINT(misc-include-cleaner)
#include <liburing/io_uring.h>

#include "hephaestus/concurrency/io_ring/io_ring_operation_pointer.h"
#include "hephaestus/concurrency/io_ring/io_ring_operation_registration.h"
#include "hephaestus/utils/exception.h"

namespace heph::concurrency::io_ring::tests {

struct alignas(alignof(void*)) TestOperation1 {
  bool prepare_called{ false };
  bool handle_completion_called{ false };

  void prepare(::io_uring_sqe* /*sqe*/) {
    prepare_called = true;
  }

  void handleCompletion(::io_uring_cqe* /*cqe*/) {
    handle_completion_called = true;
  }
};

struct alignas(alignof(void*)) TestOperation2 {
  bool prepare_called{ false };
  bool handle_completion_called{ false };

  void prepare(::io_uring_sqe* /*sqe*/) {
    prepare_called = true;
  }

  void handleCompletion(::io_uring_cqe* /*cqe*/) {
    handle_completion_called = true;
  }
};

TEST(IoRingTest, IoRingOperationRegistry) {
  IoRingOperationRegistry registry;
  EXPECT_EQ(registry.size, 0);

  auto idx1 = registry.registerOperation<TestOperation1>();
  EXPECT_EQ(idx1, 0);

  EXPECT_EQ(registry.size, 1);

  auto idx2 = registry.registerOperation<TestOperation1>();
  EXPECT_EQ(idx2, 0);

  EXPECT_EQ(registry.size, 1);

  TestOperation1 test_operation1;

  EXPECT_FALSE(test_operation1.prepare_called);
  EXPECT_FALSE(test_operation1.handle_completion_called);

  registry.prepare(0, &test_operation1, nullptr);
  EXPECT_TRUE(test_operation1.prepare_called);
  EXPECT_FALSE(test_operation1.handle_completion_called);

  registry.handleCompletion(0, &test_operation1, nullptr);
  EXPECT_TRUE(test_operation1.prepare_called);
  EXPECT_TRUE(test_operation1.handle_completion_called);

  EXPECT_THROW(registry.prepare(1, nullptr, nullptr), heph::Panic);
  EXPECT_THROW(registry.handleCompletion(1, nullptr, nullptr), heph::Panic);

  auto idx3 = registry.registerOperation<TestOperation2>();
  EXPECT_EQ(idx3, 1);

  EXPECT_EQ(registry.size, 2);

  auto idx4 = registry.registerOperation<TestOperation2>();
  EXPECT_EQ(idx4, 1);

  EXPECT_EQ(registry.size, 2);

  auto idx5 = registry.registerOperation<TestOperation1>();
  EXPECT_EQ(idx5, 0);

  TestOperation2 test_operation2;

  EXPECT_FALSE(test_operation2.prepare_called);
  EXPECT_FALSE(test_operation2.handle_completion_called);

  registry.prepare(1, &test_operation2, nullptr);
  EXPECT_TRUE(test_operation2.prepare_called);
  EXPECT_FALSE(test_operation2.handle_completion_called);

  registry.handleCompletion(1, &test_operation2, nullptr);
  EXPECT_TRUE(test_operation2.prepare_called);
  EXPECT_TRUE(test_operation2.handle_completion_called);

  EXPECT_THROW(registry.prepare(2, nullptr, nullptr), heph::Panic);
  EXPECT_THROW(registry.handleCompletion(2, nullptr, nullptr), heph::Panic);
}

TEST(IoRingTest, IoRingOperationPointer) {
  TestOperation1 test_operation1;
  const IoRingOperationPointer test_operation_ptr1(&test_operation1);
  TestOperation1 test_operation2;
  const IoRingOperationPointer test_operation_ptr2(&test_operation2);

  EXPECT_FALSE(test_operation1.prepare_called);
  EXPECT_FALSE(test_operation1.handle_completion_called);
  EXPECT_FALSE(test_operation2.prepare_called);
  EXPECT_FALSE(test_operation2.handle_completion_called);

  test_operation_ptr1.prepare(nullptr);
  EXPECT_TRUE(test_operation1.prepare_called);
  EXPECT_FALSE(test_operation1.handle_completion_called);
  EXPECT_FALSE(test_operation2.prepare_called);
  EXPECT_FALSE(test_operation2.handle_completion_called);

  test_operation_ptr1.handleCompletion(nullptr);
  EXPECT_TRUE(test_operation1.prepare_called);
  EXPECT_TRUE(test_operation1.handle_completion_called);
  EXPECT_FALSE(test_operation2.prepare_called);
  EXPECT_FALSE(test_operation2.handle_completion_called);

  test_operation_ptr2.prepare(nullptr);
  EXPECT_TRUE(test_operation1.prepare_called);
  EXPECT_TRUE(test_operation1.handle_completion_called);
  EXPECT_TRUE(test_operation2.prepare_called);
  EXPECT_FALSE(test_operation2.handle_completion_called);

  test_operation_ptr2.handleCompletion(nullptr);
  EXPECT_TRUE(test_operation1.prepare_called);
  EXPECT_TRUE(test_operation1.handle_completion_called);
  EXPECT_TRUE(test_operation2.prepare_called);
  EXPECT_TRUE(test_operation2.handle_completion_called);

  EXPECT_THROW(
      IoRingOperationRegistry::instance().prepare(IoRingOperationRegistry::instance().size, nullptr, nullptr),
      heph::Panic);
  EXPECT_THROW(IoRingOperationRegistry::instance().handleCompletion(IoRingOperationRegistry::instance().size,
                                                                    nullptr, nullptr),
               heph::Panic);
  EXPECT_THROW(IoRingOperationRegistry::instance().prepare(IoRingOperationRegistry::instance().size + 2,
                                                           nullptr, nullptr),
               heph::Panic);
  EXPECT_THROW(IoRingOperationRegistry::instance().handleCompletion(
                   IoRingOperationRegistry::instance().size + 2, nullptr, nullptr),
               heph::Panic);
}
}  // namespace heph::concurrency::io_ring::tests
