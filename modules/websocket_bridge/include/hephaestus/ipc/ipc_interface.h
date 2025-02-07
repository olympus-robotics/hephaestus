//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <absl/log/check.h>
#include <hephaestus/ipc/zenoh/dynamic_subscriber.h>
#include <hephaestus/serdes/type_info.h>

#include "hephaestus/websocket_bridge/bridge_config.h"

namespace heph::ws_bridge {

using TopicSubscriberWithTypeCallback = std::function<void(
    const ipc::zenoh::MessageMetadata&, std::span<const std::byte>, const serdes::TypeInfo&)>;

class IpcInterface {
public:
  IpcInterface(std::shared_ptr<ipc::zenoh::Session> session, WsBridgeConfig& config);

  void start();
  [[nodiscard]] std::future<void> stop();

  bool hasSubscriber(const std::string& topic) const;
  void addSubscriber(const std::string& topic, const serdes::TypeInfo& topic_type_info,
                     TopicSubscriberWithTypeCallback callback);
  void removeSubscriber(const std::string& topic);

private:
  void callback_PublisherMatchingStatus(const std::string& topic, const zenoh::MatchingStatus& status);

  std::shared_ptr<ipc::zenoh::Session> session_;
  WsBridgeConfig& config_;

  std::unordered_map<std::string, std::unique_ptr<ipc::zenoh::RawSubscriber>> subscribers_;
};

}  // namespace heph::ws_bridge