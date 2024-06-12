#pragma once

#include <random>

#include "hephaestus/random/random_container.h"
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

auto Robot::random(std::mt19937_64& mt) -> Robot {
  return { .name = random::randomT<std::string>(mt),
           .version = random::randomT<int>(mt),
           .scores = random::randomT<std::vector<float>>(mt) };
}

struct Fleet {
  auto operator==(const Fleet& other) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Fleet;

  std::string name;
  int robot_count{};
};

auto Fleet::random(std::mt19937_64& mt) -> Fleet {
  return { .name = random::randomT<std::string>(mt), .robot_count = random::randomT<int>(mt) };
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
void toProto(proto::Robot& proto_user, const Robot& user) {
  proto_user.set_name(user.name);
  proto_user.set_version(user.version);
  proto_user.mutable_scores()->Add(user.scores.begin(), user.scores.end());
}

void fromProto(const proto::Robot& proto_user, Robot& user) {
  user.name = proto_user.name();
  user.version = proto_user.version();
  user.scores = { proto_user.scores().begin(), proto_user.scores().end() };
}

void toProto(proto::Fleet& proto_company, const Fleet& company) {
  proto_company.set_name(company.name);
  proto_company.set_robot_count(company.robot_count);
}

void fromProto(const proto::Fleet& proto_company, Fleet& company) {
  company.name = proto_company.name();
  company.robot_count = proto_company.robot_count();
}

}  // namespace heph::bag::tests
