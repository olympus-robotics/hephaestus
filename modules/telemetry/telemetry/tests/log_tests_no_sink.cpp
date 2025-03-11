//================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include "hephaestus/telemetry/log.h"

TEST(LogTest, NoSinksRegisteredPrintsWarning) {
  testing::internal::CaptureStderr();
  // Restore original stderr
  heph::log(heph::INFO, "test");
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::string output = testing::internal::GetCapturedStderr();

  // Check the captured output
  std::string expected = "########################################################\n"
                         "REGISTER A LOG SINK TO SEE THE MESSAGES\n"
                         "########################################################\n\n";
  EXPECT_EQ(output, expected);
}
