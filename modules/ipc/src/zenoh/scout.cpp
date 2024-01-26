//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/scout.h"

#include <algorithm>
#include <barrier>
#include <expected>
#include <mutex>
#include <ranges>
#include <zenohc.hxx>

#include <absl/base/thread_annotations.h>
#include <nlohmann/json.hpp>
#include <zenoh.h>

#include "eolo/base/exception.h"
#include "eolo/ipc/zenoh/query.h"
#include "eolo/ipc/zenoh/utils.h"

namespace eolo::ipc::zenoh {

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
    auto values = nodes_info_ | std::views::values | std::ranges::to<std::vector<NodeInfo>>();
    return values;
  }

private:
  std::unordered_map<std::string, NodeInfo> nodes_info_ ABSL_GUARDED_BY(mutex_);
  mutable std::mutex mutex_;
};

// [[nodiscard]] auto query(zenohc::Session& session, const std::string& topic,
//                          const std::string& value) -> std::expected<std::string, std::string> {
//   std::barrier sync_point(2);
//   std::string result;
//   std::string error;
//   const auto reply_cb = [&result, &error](zenohc::Reply&& reply) {
//     const auto r = reply.get();
//     if (std::holds_alternative<zenohc::ErrorMessage>(r)) {
//       const auto msg = std::get<zenohc::ErrorMessage>(r).as_string_view();
//       error = std::format("query failed: {}", msg);
//       return;
//     }

//     result = std::get<zenohc::Sample>(r).get_payload().as_string_view();
//   };

//   auto on_done = [&sync_point] { sync_point.arrive_and_drop(); };

//   auto opts = zenohc::GetOptions();
//   opts.set_value(zenohc::Value{ value });
//   opts.set_target(Z_QUERY_TARGET_ALL);  // query all matching queryables
//   session.get(topic, "", { reply_cb, on_done }, opts);

//   sync_point.arrive_and_wait();  // wait until reply callback is triggered

//   if (!error.empty()) {
//     return std::unexpected<std::string>{ std::move(error) };
//   }

//   return result;
// }

[[nodiscard]] auto getRouterInfoJson(const std::string& router_id) -> std::string {
  zenohc::Config zconfig;
  auto session = ::eolo::ipc::zenoh::expect(open(std::move(zconfig)));

  static constexpr auto ROUTER_TOPIC = "@/router/{}";
  const auto query_topic = std::format(ROUTER_TOPIC, router_id);

  auto query_res = query(session, query_topic, "");
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
    nodes.insert_range(nodes.end(), clients);
  }

  return nodes;
}

auto toString(const NodeInfo& info) -> std::string {
  return std::format("[{}] ID: {}. Locators: {}", toString(info.mode), info.id, toString(info.locators));
}
}  // namespace eolo::ipc::zenoh
