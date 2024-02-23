//=================================================================================================
// Copyright (C) 2023-2024 EOLO Contributors
//=================================================================================================

#include <gtest/gtest.h>

#include "eolo/bag/topic_filter.h"
// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace eolo::bag::tests {

using TestCasesT = std::vector<std::pair<std::string, bool>>;

auto runTestCases(const TopicFilter& filter, const TestCasesT& test_cases) -> void {
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
    auto filter = TopicFilter::create();
    runTestCases(filter, test_cases);
  }
  {
    TopicFilterParams params;
    auto filter = TopicFilter::create(params);
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
    auto filter = TopicFilter::create().anyExcluding({ "topic" });
    runTestCases(filter, test_cases);
  }
  {
    TopicFilterParams params{ .include_topics_only = {}, .prefix = "", .exclude_topics = { "topic" } };
    auto filter = TopicFilter::create(params);
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
    auto filter = TopicFilter::create().prefix({ "hostname" });
    runTestCases(filter, test_cases);
  }
  if (false) {
    TopicFilterParams params{ .include_topics_only = {}, .prefix = "hostname", .exclude_topics = {} };
    auto filter = TopicFilter::create(params);
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
    auto filter = TopicFilter::create().prefix({ "hostname" }).anyExcluding({ "hostname/video" });
    runTestCases(filter, test_cases);
  }
  if (false) {
    TopicFilterParams params{ .include_topics_only = {},
                              .prefix = "hostname",
                              .exclude_topics = { "hostname/video" } };
    auto filter = TopicFilter::create(params);
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
    auto filter = TopicFilter::create().onlyIncluding({ "hostname/video" });
    runTestCases(filter, test_cases);
  }
  {
    TopicFilterParams params{ .include_topics_only = { "hostname/video" },
                              .prefix = "",
                              .exclude_topics = {} };
    auto filter = TopicFilter::create(params);
    runTestCases(filter, test_cases);
  }
}

}  // namespace eolo::bag::tests
