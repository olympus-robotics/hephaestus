
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/telemetry/log.h"
#include "hephaestus/utils/exception.h"

TEST(LogTest, noSinkRegistered) {
  // Call to log when no sink is registered should result in exception
  EXPECT_THROW(
      []() {
        heph::log(heph::ERROR, "test another great message");
        std::this_thread::sleep_for(std::chrono::seconds(10));
      }(),
      heph::InvalidConfigurationException);
}
