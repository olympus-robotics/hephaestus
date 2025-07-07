//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>

#include <gtest/gtest.h>
#include <stdexec/execution.hpp>

#include "hephaestus/conduit/node.h"
#include "hephaestus/conduit/node_engine.h"
#include "hephaestus/conduit/queued_input.h"
#include "hephaestus/conduit/remote_nodes.h"
#include "hephaestus/net/endpoint.h"
#include "hephaestus/telemetry/log_sink.h"
#include "hephaestus/telemetry/log_sinks/absl_sink.h"
#include "hephaestus/types/dummy_type.h"
#include "hephaestus/types_proto/dummy_type.h"  // NOLINT(misc-include-cleaner)
#include "hephaestus/utils/stack_trace.h"
#include "modules/types/include/hephaestus/types/dummy_type.h"

namespace heph::conduit::tests {

class MyEnvironment final : public testing::Environment {
public:
  ~MyEnvironment() override = default;
  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>(heph::DEBUG));
  }
  void TearDown() override {
  }

private:
  heph::utils::StackTrace stack_trace_;
};
// NOLINTNEXTLINE
const auto* const my_env = dynamic_cast<MyEnvironment*>(AddGlobalTestEnvironment(new MyEnvironment{}));

struct Generator : heph::conduit::Node<Generator> {
  static constexpr std::string_view NAME = "generator";

  static constexpr std::chrono::milliseconds PERIOD{ 10 };

  static auto trigger() {
    return stdexec::just();
  }

  static auto execute() {
    return types::DummyType{};
  }
};

struct ReceivingOperationData {
  std::size_t iterations{ 0 };
  std::size_t executed{ 0 };
  std::optional<types::DummyType> dummy;
};

struct ReceivingOperation : Node<ReceivingOperation, ReceivingOperationData> {
  QueuedInput<types::DummyType> input{ this, "input" };

  static auto trigger(ReceivingOperation& operation) {
    return operation.input.get();
  }

  static auto execute(ReceivingOperation& operation, types::DummyType dummy) {
    ++operation.data().executed;
    // operation.data().dummy.emplace(dummy);
    if (operation.data().iterations == operation.data().executed) {
      operation.engine().requestStop();
    }
    return dummy;
  }
};

TEST(RemoteNodeTests, nodeBasic) {
  NodeEngineConfig config1;
  config1.endpoints = { heph::net::Endpoint::createIpV4("127.0.0.1") };
  NodeEngine engine1{ config1 };

  NodeEngineConfig config2;
  config2.endpoints = { heph::net::Endpoint::createIpV4("127.0.0.1") };
  NodeEngine engine2{ config2 };

  auto endpoints1 = engine1.endpoints();
  auto endpoints2 = engine2.endpoints();

  // Publisher
  std::thread t1{ [&engine = engine1] {
    [[maybe_unused]] auto node = engine.createNode<Generator>();

    engine.run();
  } };

  // Subscriber
  std::thread t2{ [&engine = engine2, remote_endpoints = engine1.endpoints()] {
    static constexpr std::size_t NUM_ITERATIONS = 10;
    auto node = engine.createNode<ReceivingOperation>(NUM_ITERATIONS, 0, std::nullopt);

    EXPECT_EQ(remote_endpoints.size(), 1);
    for (const auto& endpoint : remote_endpoints) {
      auto subscriber = engine.createNode<RemoteSubscriberNode<heph::types::DummyType>>(
          endpoint, std::string{ Generator::NAME });
      node->input.connectTo(subscriber);
    }
    engine.run();
    EXPECT_EQ(node->data().executed, NUM_ITERATIONS);
    EXPECT_EQ(node->data().iterations, NUM_ITERATIONS);
  } };
  t2.join();
  engine1.requestStop();
  t1.join();
}

TEST(RemoteNodeTests, subscriberRestart) {
  NodeEngineConfig config1;
  config1.endpoints = { heph::net::Endpoint::createIpV4("127.0.0.1") };
  NodeEngine engine1{ config1 };

  // Publisher
  std::thread t1{ [&engine = engine1] {
    [[maybe_unused]] auto node = engine.createNode<Generator>();

    engine.run();
  } };

  // Subscriber
  std::thread t2{ [remote_endpoints = engine1.endpoints()] {
    static constexpr std::size_t NUM_ITERATIONS = 100;
    for (std::size_t i = 0; i != NUM_ITERATIONS; ++i) {
      NodeEngine engine{ {} };
      auto node = engine.createNode<ReceivingOperation>(1, 0, std::nullopt);

      EXPECT_EQ(remote_endpoints.size(), 1);
      for (const auto& endpoint : remote_endpoints) {
        auto subscriber = engine.createNode<RemoteSubscriberNode<heph::types::DummyType>>(
            endpoint, std::string{ Generator::NAME });
        node->input.connectTo(subscriber);
      }
      engine.run();
      EXPECT_EQ(node->data().executed, 1);
      EXPECT_EQ(node->data().iterations, 1);
    }
  } };

  t2.join();
  engine1.requestStop();
  t1.join();
}

TEST(RemoteNodeTests, publisherRestart) {
  static constexpr std::size_t NUM_ITERATIONS = 100;
  std::mutex endpoint_mtx;
  std::condition_variable endpoint_cv;
  std::optional<heph::net::Endpoint> endpoint;

  NodeEngine subscriber_engine{ {} };
  // Publisher
  std::thread t1{ [&] {
    for (std::size_t i = 0; i != NUM_ITERATIONS; ++i) {
      NodeEngineConfig config;
      {
        const std::scoped_lock l{ endpoint_mtx };
        if (!endpoint.has_value()) {
          config.endpoints = { heph::net::Endpoint::createIpV4("127.0.0.1") };

        } else {
          config.endpoints = { endpoint.value() };
        }
      }
      try {
        NodeEngine engine{ config };

        {
          const std::scoped_lock l{ endpoint_mtx };
          endpoint.emplace(engine.endpoints().front());
          endpoint_cv.notify_all();
        }

        auto generator = engine.createNode<Generator>();
        auto stopper = engine.createNode<ReceivingOperation>(NUM_ITERATIONS, 0, std::nullopt);

        stopper->input.connectTo(generator);

        engine.run();

      } catch (std::exception& exception) {
        std::this_thread::yield();
      }
    }
  } };

  // Subscriber
  std::thread t2{ [&] {
    auto node =
        subscriber_engine.createNode<ReceivingOperation>(NUM_ITERATIONS * NUM_ITERATIONS, 0, std::nullopt);

    heph::net::Endpoint remote_endpoint;
    {
      std::unique_lock l{ endpoint_mtx };
      endpoint_cv.wait(l, [&]() { return endpoint.has_value(); });
      remote_endpoint = endpoint.value();
    }

    auto subscriber = subscriber_engine.createNode<RemoteSubscriberNode<heph::types::DummyType>>(
        remote_endpoint, std::string{ Generator::NAME });
    node->input.connectTo(subscriber);
    subscriber_engine.run();
    EXPECT_GT(node->data().executed, 0);
    EXPECT_LT(node->data().executed, NUM_ITERATIONS * NUM_ITERATIONS);
  } };

  t1.join();
  subscriber_engine.requestStop();
  t2.join();
}

}  // namespace heph::conduit::tests
