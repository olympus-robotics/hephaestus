
//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>  // NOLINT(misc-include-cleaner)
#include <nlohmann/json_fwd.hpp>

#include "hephaestus/ipc/zenoh/session.h"

TEST(ZenohTests, ConfigSetSessionId) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::setSessionId(config, "blubb");

  const nlohmann::json new_config1 = config.zconfig.to_string();

  heph::ipc::zenoh::setSessionIdFromBinary(config);

  const nlohmann::json new_config2 = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config1);
  ASSERT_NE(default_config, new_config2);
  ASSERT_NE(new_config1, new_config2);
}

TEST(ZenohTests, ConfigSetSharedMemory) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::setSharedMemory(config, false);

  const nlohmann::json new_config = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config);
}

TEST(ZenohTests, ConfigSetMode) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::setMode(config, heph::ipc::zenoh::Mode::CLIENT);
  const nlohmann::json new_config1 = config.zconfig.to_string();

  heph::ipc::zenoh::setMode(config, heph::ipc::zenoh::Mode::ROUTER);
  const nlohmann::json new_config2 = config.zconfig.to_string();

  heph::ipc::zenoh::setMode(config, heph::ipc::zenoh::Mode::PEER);
  const nlohmann::json new_config3 = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config1);
  ASSERT_NE(default_config, new_config2);
  ASSERT_NE(default_config, new_config3);
  ASSERT_NE(new_config1, new_config2);
  ASSERT_NE(new_config1, new_config3);
}

TEST(ZenohTests, ConnectToEndpoints) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::connectToEndpoints(config, { "tcp/0.0.0.0:7447", "udp/localhost:7448" });

  const nlohmann::json new_config = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config);
}

TEST(ZenohTests, ListenToEndpoints) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::listenToEndpoints(config, { "tcp/0.0.0.0:7447", "udp/localhost:7448" });

  const nlohmann::json new_config = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config);
}

TEST(ZenohTests, ConfigSetQos) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::setQos(config, false);

  const nlohmann::json new_config = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config);
}

TEST(ZenohTests, ConfigSetRealTime) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::setRealTime(config, true);

  const nlohmann::json new_config = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config);
}

TEST(ZenohTests, ConfigSetMulticastScouting) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::setMulticastScouting(config, true);

  const nlohmann::json new_config = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config);
}

TEST(ZenohTests, ConfigSetMulticastScoutingInterface) {
  heph::ipc::zenoh::ZenohConfig config;

  const nlohmann::json default_config = config.zconfig.to_string();

  heph::ipc::zenoh::setMulticastScoutingInterface(config, "lilo");

  const nlohmann::json new_config = config.zconfig.to_string();

  ASSERT_NE(default_config, new_config);
}
