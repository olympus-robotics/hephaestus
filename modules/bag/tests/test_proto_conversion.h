#pragma once

#include <random>

#include "hephaestus/random/random_container.h"
#include "hephaestus/serdes/protobuf/concepts.h"
#include "test.pb.h"

namespace heph::bag::tests {
struct User {
  auto operator==(const User& other) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> User;

  std::string name;
  int age{};
  std::vector<float> scores;
};

auto User::random(std::mt19937_64& mt) -> User {
  return { .name = random::randomT<std::string>(mt),
           .age = random::randomT<int>(mt),
           .scores = random::randomT<std::vector<float>>(mt) };
}

struct Company {
  auto operator==(const Company& other) const -> bool = default;
  [[nodiscard]] static auto random(std::mt19937_64& mt) -> Company;

  std::string name;
  int employer_count{};
};

auto Company::random(std::mt19937_64& mt) -> Company {
  return { .name = random::randomT<std::string>(mt), .employer_count = random::randomT<int>(mt) };
}
}  // namespace heph::bag::tests

namespace heph::serdes::protobuf {
template <>
struct ProtoAssociation<heph::bag::tests::User> {
  using Type = heph::bag::tests::proto::User;
};

template <>
struct ProtoAssociation<heph::bag::tests::Company> {
  using Type = heph::bag::tests::proto::Company;
};
}  // namespace heph::serdes::protobuf

namespace heph::bag::tests {
void toProto(proto::User& proto_user, const User& user) {
  proto_user.set_name(user.name);
  proto_user.set_age(user.age);
  proto_user.mutable_scores()->Add(user.scores.begin(), user.scores.end());
}

void fromProto(const proto::User& proto_user, User& user) {
  user.name = proto_user.name();
  user.age = proto_user.age();
  user.scores = { proto_user.scores().begin(), proto_user.scores().end() };
}

void toProto(proto::Company& proto_company, const Company& company) {
  proto_company.set_name(company.name);
  proto_company.set_employer_count(company.employer_count);
}

void fromProto(const proto::Company& proto_company, Company& company) {
  company.name = proto_company.name();
  company.employer_count = proto_company.employer_count();
}

}  // namespace heph::bag::tests
