//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#include <chrono>
#include <cstdint>
#include <span>
#include <thread>
#include <vector>

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <hephaestus/ipc/zenoh/service.h>
#include <hephaestus/ipc/zenoh/session.h>
#include <hephaestus/random/random_number_generator.h>
#include <hephaestus/random/random_object_creator.h>
#include <hephaestus/serdes/type_info.h>
#include <hephaestus/telemetry/log.h>
#include <hephaestus/telemetry/log_sinks/absl_sink.h>
#include <hephaestus/types/dummy_type.h>
#include <hephaestus/types_proto/dummy_type.h>

#include "hephaestus/ipc/ipc_interface.h"

namespace heph::ws::tests {

class IpcInterfaceTest : public testing::Test {
protected:
  // NOLINTBEGIN
  std::shared_ptr<heph::ipc::zenoh::Session> session_;
  heph::ipc::zenoh::Config config_;
  std::unique_ptr<heph::ws::IpcInterface> ipc_interface_;
  std::unique_ptr<heph::ipc::zenoh::Service<types::DummyType, types::DummyType>> service_server_;
  // NOLINTEND

  void SetUp() override {
    heph::telemetry::registerLogSink(std::make_unique<heph::telemetry::AbslLogSink>());

    config_.id = "ws_bridge";
    config_.multicast_scouting_enabled = true;

    session_ = heph::ipc::zenoh::createSession(config_);
    ipc_interface_ = std::make_unique<heph::ws::IpcInterface>(session_, config_);

    static constexpr int TIMEOUT_MS = 20;

    // Set up a service server
    heph::ipc::TopicConfig service_config{ "test_service" };
    service_server_ = std::make_unique<heph::ipc::zenoh::Service<types::DummyType, types::DummyType>>(
        session_, service_config, [](const types::DummyType& request) -> types::DummyType {
          // It simply sleeps for a bit and throws the same request back, whatever it is.
          std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
          return request;
        });

    ipc_interface_->start();
  }

  void TearDown() override {
    ipc_interface_->stop();
    service_server_.reset();
  }
};

TEST_F(IpcInterfaceTest, AddSubscriber) {
  std::string topic = "test_topic";
  heph::serdes::TypeInfo type_info;
  bool callback_called = false;

  ipc_interface_->addSubscriber(topic, type_info,
                                [&](const heph::ipc::zenoh::MessageMetadata&, std::span<const std::byte>,
                                    const heph::serdes::TypeInfo&) { callback_called = true; });

  EXPECT_TRUE(ipc_interface_->hasSubscriber(topic));
}

TEST_F(IpcInterfaceTest, RemoveSubscriber) {
  std::string topic = "test_topic";
  serdes::TypeInfo type_info;

  ipc_interface_->addSubscriber(topic, type_info,
                                [](const heph::ipc::zenoh::MessageMetadata&, std::span<const std::byte>,
                                   const heph::serdes::TypeInfo&) {});
  ipc_interface_->removeSubscriber(topic);

  EXPECT_FALSE(ipc_interface_->hasSubscriber(topic));
}

TEST_F(IpcInterfaceTest, HasSubscriber) {
  std::string topic = "test_topic";
  serdes::TypeInfo type_info;

  EXPECT_FALSE(ipc_interface_->hasSubscriber(topic));

  ipc_interface_->addSubscriber(
      topic, type_info,
      [](const ipc::zenoh::MessageMetadata&, std::span<const std::byte>, const serdes::TypeInfo&) {});

  EXPECT_TRUE(ipc_interface_->hasSubscriber(topic));
}

TEST_F(IpcInterfaceTest, CallService) {
  std::string topic = "test_service";
  ipc::TopicConfig topic_config{ topic };

  auto mt = random::createRNG();
  const auto request_message = types::DummyType::random(mt);
  auto request_buffer = serdes::serialize(request_message);

  static constexpr int TIMEOUT_MS = 1000;

  std::chrono::milliseconds timeout(TIMEOUT_MS);

  const uint32_t call_id = 42;

  auto responses = ipc_interface_->callService(call_id, topic_config, request_buffer, timeout);

  EXPECT_FALSE(responses.empty());
  ASSERT_EQ(responses.size(), 1);
  EXPECT_EQ(responses.front().topic, topic);

  types::DummyType reply;
  auto reply_buffer = responses.front().value;
  serdes::deserialize<types::DummyType>(reply_buffer, reply);

  EXPECT_EQ(reply, request_message);
}

TEST_F(IpcInterfaceTest, CallServiceAsync) {
  std::string topic = "test_service";
  ipc::TopicConfig topic_config{ topic };

  auto mt = random::createRNG();
  const auto request_message = types::DummyType::random(mt);
  auto request_buffer = serdes::serialize(request_message);

  static constexpr int TIMEOUT_MS = 1000;

  std::chrono::milliseconds timeout(TIMEOUT_MS);
  bool callback_called = false;
  heph::log(heph::INFO, "[IPC Interface TEST] - Calling ASYNC service", "topic", topic);

  const uint32_t call_id = 42;
  auto future = ipc_interface_->callServiceAsync(call_id, topic_config, request_buffer, timeout,
                                                 [&](const RawServiceResponses& responses) {
                                                   callback_called = true;
                                                   EXPECT_FALSE(responses.empty());
                                                   ASSERT_EQ(responses.size(), 1);
                                                   EXPECT_EQ(responses.front().topic, topic);

                                                   types::DummyType reply;
                                                   auto reply_buffer = responses.front().value;
                                                   serdes::deserialize<types::DummyType>(reply_buffer, reply);

                                                   EXPECT_EQ(reply, request_message);
                                                 });
  heph::log(heph::INFO, "[IPC Interface TEST] - Call dispached. Waiting for async call.");

  future.wait();

  EXPECT_TRUE(callback_called);
}

}  // namespace heph::ws::tests