//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include "eolo/bag/topic_filter.h"

#include <unordered_set>

namespace eolo::bag {

auto TopicFilter::create() -> TopicFilter {
  return {};
}

auto TopicFilter::create(const TopicFilterParams& params) -> TopicFilter {
  if (!params.include_topics_only.empty()) {
    return create().onlyIncluding(params.include_topics_only);
  }

  TopicFilter filter = create();
  if (!params.exclude_topics.empty()) {
    filter = std::move(filter).anyExcluding(params.exclude_topics);
  }

  if (!params.prefix.empty()) {
    filter = std::move(filter).prefix(params.prefix);
  }

  return filter;
}

auto TopicFilter::onlyIncluding(const std::vector<std::string>& topic_names) && -> TopicFilter {
  match_cb_.clear();
  include_only_filter_set_ = true;
  std::unordered_set<std::string> including_topic{ topic_names.begin(), topic_names.end() };
  match_cb_.emplace_back([including_topic = std::move(including_topic)](const auto& topic) mutable {
    return including_topic.contains(topic);
  });
  return std::move(*this);
}

auto TopicFilter::prefix(std::string prefix) && -> TopicFilter {
  match_cb_.emplace_back([prefix = std::move(prefix)](const auto& topic) {
    if (prefix == "**") {
      return true;
    }

    return topic.starts_with(prefix);
  });
  return std::move(*this);
}

auto TopicFilter::anyExcluding(const std::vector<std::string>& topic_names) && -> TopicFilter {
  std::unordered_set<std::string> excluding_topic{ topic_names.begin(), topic_names.end() };
  match_cb_.emplace_back([excluding_topic = std::move(excluding_topic)](const auto& topic) mutable {
    return !excluding_topic.contains(topic);
  });
  return std::move(*this);
}

/// Return true if the input topic passes the concatenated list of filters.
auto TopicFilter::isAcceptable(const std::string& topic) const -> bool {
  return std::all_of(match_cb_.begin(), match_cb_.end(), [&topic](const auto& cb) { return cb(topic); });
}

}  // namespace eolo::bag
