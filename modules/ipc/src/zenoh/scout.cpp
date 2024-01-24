//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/ipc/zenoh/scout.h"

#include <mutex>
#include <ranges>
#include <zenohc.hxx>

#include <absl/base/thread_annotations.h>
#include <zenoh.h>

#include "eolo/base/exception.h"
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

}  // namespace

auto getListOfNodes() -> std::vector<NodeInfo> {
  ScoutDataManager manager;

  zenohc::ScoutingConfig config;
  const auto success =
      zenohc::scout(std::move(config), [&manager](const auto& hello) { manager.onDiscovery(hello); });
  throwExceptionIf<FailedZenohOperation>(!success, "zenoh scouting failed to get list of nodes");

  return manager.getNodesInfo();
}

auto toString(const NodeInfo& info) -> std::string {
  return std::format("[{}] ID: {}. Locators: {}", toString(info.mode), info.id, toString(info.locators));
}
}  // namespace eolo::ipc::zenoh
