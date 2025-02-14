#include <gtest/gtest.h>

#include "hephaestus/ipc/ipc_interface.h"

namespace heph::ws_bridge::tests {

class IpcInterfaceTest : public testing::Test {
protected:
  std::shared_ptr<heph::ipc::zenoh::Session> session_;
  std::unique_ptr<heph::ws_bridge::IpcInterface> ipc_interface_;

  void SetUp() override {
    session_ = heph::ipc::zenoh::createSession(heph::ipc::zenoh::createLocalConfig());
    ipc_interface_ = std::make_unique<heph::ws_bridge::IpcInterface>(session_);
  }

  void TearDown() override {
    ipc_interface_->stop();
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

}  // namespace heph::ws_bridge::tests