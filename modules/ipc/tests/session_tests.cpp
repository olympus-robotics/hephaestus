#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "hephaestus/ipc/zenoh/conversions.h"
#include "hephaestus/ipc/zenoh/session.h"
#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/test_utils/heph_test.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::ipc::zenoh::tests {

namespace {

struct Session : heph::test_utils::HephTest {};

TEST_F(Session, SessionId) {
  static constexpr auto MAX_ID_LENGTH = 16;

  auto config = Config{};
  config.id = random::random<std::string>(mt, MAX_ID_LENGTH, false, true);

  auto session = createSession(config);
  EXPECT_EQ(toString(session->zenoh_session.get_zid()), config.id);
}

}  // namespace
}  // namespace heph::ipc::zenoh::tests
