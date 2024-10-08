//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/concurrency/message_queue_consumer.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/utils/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::concurrency::tests {

struct Message {
  [[nodiscard]] auto operator==(const Message&) const -> bool = default;
  int value;
};

TEST(MessageQueueConsumer, Fail) {
#ifndef DISABLE_EXCEPTION
  EXPECT_THROW(std::ignore = MessageQueueConsumer<Message>([](const Message&) {}, 0),
               InvalidParameterException);
#endif
}

TEST(MessageQueueConsumer, ProcessMessages) {
  static constexpr std::size_t MESSAGE_COUNT = 2;
  std::atomic_flag flag = ATOMIC_FLAG_INIT;
  std::vector<Message> processed_messages;

  MessageQueueConsumer<Message> spinner{ [&processed_messages, &flag](const Message& message) {
                                          processed_messages.push_back(message);
                                          if (processed_messages.size() == MESSAGE_COUNT) {
                                            flag.test_and_set();
                                            flag.notify_all();
                                          }
                                        },
                                         MESSAGE_COUNT };
  spinner.start();
  auto mt = random::createRNG();
  std::vector<Message> messages(MESSAGE_COUNT);
  std::ranges::for_each(messages, [&mt](Message& message) { message.value = random::random<int>(mt); });
  for (const auto& message : messages) {
    spinner.queue().forcePush(message);
  }

  flag.wait(false);

  EXPECT_EQ(messages, processed_messages);

  spinner.stop();
}

}  // namespace heph::concurrency::tests
