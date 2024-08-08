#pragma once

#include <random>

#include "hephaestus/random/random_object_creator.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "test.pb.h"

namespace heph::bag::tests {
struct Robot {
  auto operator==(const Robot& other) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Robot;

  std::string name;
  int version{};
  std::vector<float> scores;
};

inline auto Robot::random(std::mt19937_64& mt) -> Robot {
  return { .name = random::random<std::string>(mt),
           .version = random::random<int>(mt),
           .scores = random::random<std::vector<float>>(mt) };
}

struct Fleet {
  auto operator==(const Fleet& other) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Fleet;

  std::string name;
  int robot_count{};
};

inline auto Fleet::random(std::mt19937_64& mt) -> Fleet {
  return { .name = random::random<std::string>(mt), .robot_count = random::random<int>(mt) };
}
}  // namespace heph::bag::tests

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::bag::tests::Robot> {
  using Type = heph::bag::tests::proto::Robot;
};

template <>
struct ProtoAssociation<heph::bag::tests::Fleet> {
  using Type = heph::bag::tests::proto::Fleet;
};
}  // namespace heph::serdes::protobuf

namespace heph::bag::tests {
inline void toProto(proto::Robot& proto_user, const Robot& user) {
  proto_user.set_name(user.name);
  proto_user.set_version(user.version);
  proto_user.mutable_scores()->Add(user.scores.begin(), user.scores.end());
}

inline void fromProto(const proto::Robot& proto_user, Robot& user) {
  user.name = proto_user.name();
  user.version = proto_user.version();
  user.scores = { proto_user.scores().begin(), proto_user.scores().end() };
}

inline void toProto(proto::Fleet& proto_company, const Fleet& company) {
  proto_company.set_name(company.name);
  proto_company.set_robot_count(company.robot_count);
}

inline void fromProto(const proto::Fleet& proto_company, Fleet& company) {
  company.name = proto_company.name();
  company.robot_count = proto_company.robot_count();
}

}  // namespace heph::bag::tests
