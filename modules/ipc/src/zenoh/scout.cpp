//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include "hephaestus/ipc/zenoh/scout.h"

#include <algorithm>
#include <mutex>

#include <absl/base/thread_annotations.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <range/v3/range/conversion.hpp>
#include <zenoh.h>
#include <zenohc.hxx>

#include "hephaestus/base/exception.h"
#include "hephaestus/ipc/zenoh/service.h"
#include "hephaestus/ipc/zenoh/utils.h"

namespace heph::ipc::zenoh {

namespace {

class ScoutDataManager {
public:
  void onDiscovery(const zenohc::HelloView& hello) {
    std::unique_lock<std::mutex> lock{ mutex_ };
    auto id = toString(hello.get_id());
    nodes_info_.emplace(id, NodeInfo{ .id = id,
                                      .mode = toMode(hello.get_whatami()),
                                      .locators = toStringVector(hello.get_locators()) });
  }

  [[nodiscard]] auto getNodesInfo() const -> std::vector<NodeInfo> {
    std::unique_lock<std::mutex> lock{ mutex_ };
    auto values = nodes_info_ | std::views::values | ranges::to<std::vector<NodeInfo>>();
    return values;
  }

private:
  std::unordered_map<std::string, NodeInfo> nodes_info_ ABSL_GUARDED_BY(mutex_);
  mutable std::mutex mutex_;
};

[[nodiscard]] auto getRouterInfoJson(const std::string& router_id) -> std::string {
  heph::ipc::Config config;
  auto session = heph::ipc::zenoh::createSession(std::move(config));

  static constexpr auto ROUTER_TOPIC = "@/router/{}";
  const auto query_topic = TopicConfig{ fmt::format(ROUTER_TOPIC, router_id) };
  fmt::println("QUERY TOPIC: {}", query_topic.name);
  auto query_res = callService<std::string, std::string>(session, query_topic, "");
  throwExceptionIf<FailedZenohOperation>(query_res.empty(), "failed to query for router info: no response");

  return query_res.front().value;
}

[[nodiscard]] auto getListOfClientsFromRouter(const std::string& router_id) -> std::vector<NodeInfo> {
  const auto router_info = getRouterInfoJson(router_id);

  if (router_info.empty()) {
    return {};
  }

  std::vector<NodeInfo> nodes_info;
  auto data = nlohmann::json::parse(router_info);
  const auto& sessions = data["sessions"];
  for (const auto& session : sessions) {
    if (session["whatami"] == "client") {
      nodes_info.emplace_back(session["peer"], Mode::CLIENT, session["links"]);
    }
  }

  return nodes_info;
}

}  // namespace

auto getListOfNodes() -> std::vector<NodeInfo> {
  ScoutDataManager manager;

  zenohc::ScoutingConfig config;
  const auto success =
      zenohc::scout(std::move(config), [&manager](const auto& hello) { manager.onDiscovery(hello); });
  throwExceptionIf<FailedZenohOperation>(!success, "zenoh scouting failed to get list of nodes");
  auto nodes = manager.getNodesInfo();

  // Scout returns only peers and routers if we want the clients we need to query the router.
  auto router = std::ranges::find_if(nodes, [](const auto& node) { return node.mode == Mode::ROUTER; });
  if (router != nodes.end()) {
    auto clients = getListOfClientsFromRouter(router->id);
    nodes.insert(nodes.end(), clients.begin(), clients.end());
  }

  return nodes;
}

auto toString(const NodeInfo& info) -> std::string {
  return fmt::format("[{}] ID: {}. Locators: {}", toString(info.mode), info.id, toString(info.locators));
}
}  // namespace heph::ipc::zenoh
