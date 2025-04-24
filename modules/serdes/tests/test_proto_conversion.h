//=================================================================================================
// Copyright (C) 2025 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <string>
#include <vector>

#include "hephaestus/serdes/protobuf/concepts.h"
#include "test_user_type.pb.h"

namespace heph::serdes {
namespace tests {
struct User {
  auto operator==(const User& other) const -> bool = default;

  std::string name;
  int age{};
  std::vector<float> scores;
};
}  // namespace tests

namespace protobuf {
template <>
struct ProtoAssociation<tests::User> {
  using Type = tests::proto::User;
};
}  // namespace protobuf

namespace tests {
inline void toProto(proto::User& proto_user, const User& user) {
  proto_user.set_name(user.name);
  proto_user.set_age(user.age);
  proto_user.mutable_scores()->Add(user.scores.begin(), user.scores.end());
}

inline void fromProto(const proto::User& proto_user, User& user) {
  user.name = proto_user.name();
  user.age = proto_user.age();
  user.scores = { proto_user.scores().begin(), proto_user.scores().end() };
}
}  // namespace tests

}  // namespace heph::serdes
