#include <memory>
#include <string>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/liveliness.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_number_generator.h"
#include "hephaestus/random/random_object_creator.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::tests {

namespace {

TEST(Session, SessionId) {
  static constexpr auto MAX_ID_LENGTH = 16;
  auto mt = random::createRNG();
  auto config = Config{};
  config.id = random::random<std::string>(mt, MAX_ID_LENGTH, false, true);

  auto session = createSession(config);
  EXPECT_EQ(toString(session->zenoh_session.get_zid()), config.id);
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
