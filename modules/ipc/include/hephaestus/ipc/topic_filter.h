//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace heph::ipc {

struct TopicFilterParams {
  // TODO: add regex

  std::vector<std::string> include_topics_only;  //!< If specified only the topics in this list are
                                                 //!< going to be recorded. This rule has precedence
                                                 //!< over all the other
  std::string prefix;                            //!< Record all the topic sharing the prefix
  std::string exclude_prefix;                    //!< Exclude all topics that share the prefix
  std::vector<std::string> exclude_topics;       //!< List of topics to exclude
};

class TopicFilter {
public:
  [[nodiscard]] static auto create() -> TopicFilter;
  [[nodiscard]] static auto create(const TopicFilterParams& params) -> TopicFilter;

  /// If specified this is the only one allowed, all the filter specified before will be removed and
  /// no other filter can be specified after.
  [[nodiscard]] auto onlyIncluding(const std::vector<std::string>& topic_names) && -> TopicFilter;

  [[nodiscard]] auto prefix(std::string prefix) && -> TopicFilter;
  [[nodiscard]] auto excludePrefix(std::string prefix) && -> TopicFilter;

  [[nodiscard]] auto anyExcluding(const std::vector<std::string>& topic_names) && -> TopicFilter;

  /// Return true if the input topic passes the concatenated list of filters.
  [[nodiscard]] auto isAcceptable(const std::string& topic) const -> bool;

private:
  TopicFilter() = default;

private:
  using MatchCallback = std::function<bool(const std::string&)>;
  std::vector<MatchCallback> match_cb_;

  bool include_only_filter_set_ = false;
};
}  // namespace heph::ipc
