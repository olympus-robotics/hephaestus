//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/ipc/topic_filter.h"
// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::bag::tests {
namespace {
using TestCasesT = std::vector<std::pair<std::string, bool>>;

auto runTestCases(const ipc::TopicFilter& filter, const TestCasesT& test_cases) -> void {
  for (const auto& [input, expected_output] : test_cases) {
    EXPECT_EQ(filter.isAcceptable(input), expected_output) << "input: " << input;
  }
}

TEST(TopicFilter, NoFilters) {
  const TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", true },
  };
  {
    const auto filter = ipc::TopicFilter::create();
    runTestCases(filter, test_cases);
  }
  {
    const ipc::TopicFilterParams params;
    const auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, AnyExcluding) {
  const TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", false },
  };
  {
    const auto filter = ipc::TopicFilter::create().anyExcluding({ "topic" });
    runTestCases(filter, test_cases);
  }
  {
    const ipc::TopicFilterParams params{ .include_topics_only = {},
                                         .prefix = "",
                                         .exclude_topics = { "topic" } };
    const auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, Prefix) {
  const TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", false },
  };

  {
    const auto filter = ipc::TopicFilter::create().prefix({ "hostname" });
    runTestCases(filter, test_cases);
  }
  {
    const ipc::TopicFilterParams params{ .include_topics_only = {},
                                         .prefix = "hostname",
                                         .exclude_topics = {} };
    const auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, PrefixWildcard) {
  const TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", true },
  };

  {
    const auto filter = ipc::TopicFilter::create().prefix({ "**" });
    runTestCases(filter, test_cases);
  }
  {
    const ipc::TopicFilterParams params{ .include_topics_only = {}, .prefix = "**", .exclude_topics = {} };
    const auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, PrefixAndExcluding) {
  const TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", false },
    { "topic", false },
  };
  {
    const auto filter = ipc::TopicFilter::create().prefix({ "hostname" }).anyExcluding({ "hostname/video" });
    runTestCases(filter, test_cases);
  }
  {
    const ipc::TopicFilterParams params{ .include_topics_only = {},
                                         .prefix = "hostname",
                                         .exclude_topics = { "hostname/video" } };
    const auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, IncludeOnly) {
  const TestCasesT test_cases{
    { "hostname/image", false },
    { "hostname/video", true },
    { "topic", false },
  };
  {
    const auto filter = ipc::TopicFilter::create().onlyIncluding({ "hostname/video" });
    runTestCases(filter, test_cases);
  }
  {
    const ipc::TopicFilterParams params{ .include_topics_only = { "hostname/video" },
                                         .prefix = "",
                                         .exclude_topics = {} };
    auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}
}  // namespace
}  // namespace heph::bag::tests
