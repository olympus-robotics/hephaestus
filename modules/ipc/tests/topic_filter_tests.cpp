//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "hephaestus/ipc/topic_filter.h"
// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::bag::tests {

using TestCasesT = std::vector<std::pair<std::string, bool>>;

auto runTestCases(const ipc::TopicFilter& filter, const TestCasesT& test_cases) -> void {
  for (const auto& [input, expected_output] : test_cases) {
    EXPECT_EQ(filter.isAcceptable(input), expected_output) << "input: " << input;
  }
}

TEST(TopicFilter, NoFilters) {
  TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", true },
  };
  {
    auto filter = ipc::TopicFilter::create();
    runTestCases(filter, test_cases);
  }
  {
    ipc::TopicFilterParams params;
    auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, AnyExcluding) {
  TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", false },
  };
  {
    auto filter = ipc::TopicFilter::create().anyExcluding({ "topic" });
    runTestCases(filter, test_cases);
  }
  {
    ipc::TopicFilterParams params{ .include_topics_only = {}, .prefix = "", .exclude_topics = { "topic" } };
    auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, Prefix) {
  TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", false },
  };

  {
    auto filter = ipc::TopicFilter::create().prefix({ "hostname" });
    runTestCases(filter, test_cases);
  }
  {
    ipc::TopicFilterParams params{ .include_topics_only = {}, .prefix = "hostname", .exclude_topics = {} };
    auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, PrefixWildcard) {
  TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", true },
    { "topic", true },
  };

  {
    auto filter = ipc::TopicFilter::create().prefix({ "**" });
    runTestCases(filter, test_cases);
  }
  {
    ipc::TopicFilterParams params{ .include_topics_only = {}, .prefix = "**", .exclude_topics = {} };
    auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, PrefixAndExcluding) {
  TestCasesT test_cases{
    { "hostname/image", true },
    { "hostname/video", false },
    { "topic", false },
  };
  {
    auto filter = ipc::TopicFilter::create().prefix({ "hostname" }).anyExcluding({ "hostname/video" });
    runTestCases(filter, test_cases);
  }
  {
    ipc::TopicFilterParams params{ .include_topics_only = {},
                                   .prefix = "hostname",
                                   .exclude_topics = { "hostname/video" } };
    auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

TEST(TopicFilter, IncludeOnly) {
  TestCasesT test_cases{
    { "hostname/image", false },
    { "hostname/video", true },
    { "topic", false },
  };
  {
    auto filter = ipc::TopicFilter::create().onlyIncluding({ "hostname/video" });
    runTestCases(filter, test_cases);
  }
  {
    ipc::TopicFilterParams params{ .include_topics_only = { "hostname/video" },
                                   .prefix = "",
                                   .exclude_topics = {} };
    auto filter = ipc::TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

}  // namespace heph::bag::tests
