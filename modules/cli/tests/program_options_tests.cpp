//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "hephaestus/cli/program_options.h"
#include "hephaestus/utils/exception.h"

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace ::testing;

namespace heph::cli::tests {

TEST(ProgramOptions, Empty) {
  auto desc = ProgramDescription("A dummy service that does nothing");
  auto options = std::move(desc).parse({});

  EXPECT_TRUE(options.hasOption("help"));
}

TEST(ProgramOptions, Option) {
  static constexpr float NUMBER = 1.1f;
  auto desc = ProgramDescription("A dummy service that does nothing");
  desc.defineOption<std::string>("option", "desc")
      .defineOption<std::string>("other_option", 'o', "other desc")
      .defineOption<float>("bar", "desc", NUMBER)
      .defineOption<int>("foo", 'f', "desc", 1)
      .defineFlag("flag", 'b', "desc");
  {
    auto options =
        ProgramDescription{ desc }.parse({ "--option", "value", "-o", "other_value", "--bar", "1.2" });
    EXPECT_TRUE(options.hasOption("option"));
    EXPECT_EQ(options.getOption<std::string>("option"), "value");
    EXPECT_TRUE(options.hasOption("other_option"));
    EXPECT_EQ(options.getOption<std::string>("other_option"), "other_value");
    EXPECT_TRUE(options.hasOption("bar"));
    EXPECT_EQ(options.getOption<float>("bar"), 1.2f);
    EXPECT_TRUE(options.hasOption("foo"));
    EXPECT_EQ(options.getOption<int>("foo"), 1);
    EXPECT_TRUE(options.hasOption("flag"));
    EXPECT_FALSE(options.getOption<bool>("flag"));
  }
  {
    auto options = ProgramDescription{ desc }.parse(
        { "--option", "value", "-o", "other_value", "--bar", "1.2", "--flag" });
    EXPECT_TRUE(options.hasOption("flag"));
    EXPECT_TRUE(options.getOption<bool>("flag"));
  }
}

TEST(ProgramOptions, Errors) {
  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    EXPECT_THROW(std::move(desc).parse({ "--option" }), InvalidParameterException);
  }

  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    desc.defineOption<std::string>("option", "desc").defineOption<int>("other", "desc");
    EXPECT_THROW(ProgramDescription{ desc }.parse({}), InvalidConfigurationException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option" }), InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "value" }), InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option", "--other_option" }),
                 InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option", "value", "other_value" }),
                 InvalidParameterException);
    EXPECT_THROW(ProgramDescription{ desc }.parse({ "--option", "value" }), InvalidConfigurationException);
  }

  {
    auto desc = ProgramDescription("A dummy service that does nothing");
    desc.defineOption<int>("option", "desc");
    auto options = ProgramDescription{ desc }.parse({ "--option", "1.2" });
    auto v = options.getOption<int>("option");
    EXPECT_EQ(v, 1);
  }
}

}  // namespace heph::cli::tests
